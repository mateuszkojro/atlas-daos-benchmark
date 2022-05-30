#include "DAOSZipkinLog.h"
#include "EventQueue.h"
#include "MockPool.h"
#include "Pool.h"
#include "UUID.h"
#include "daos_types.h"
#include "interfaces.h"
#include <atomic>
#include <benchmark/benchmark.h>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#define UNUSED_RANGE benchmark::CreateDenseRange(1, 1, 1)

std::string POOL_LABEL = "mkojro";
// UUID POOL_UUID("3cb8ac1e-f0c0-4aa1-8cd2-af72b5e44c17");
int MIN_CHUNK_SIZE = 1024;
int MAX_CHUNK_SIZE = 10 * 1024;
int CHUNK_SIZE_STEP = 512;
int INFLIGH_EVENTS_MIN = 2;
int INFLIGH_EVENTS_MAX = 200;
int INFLIGH_EVENTS_STEP = 50;

int REPETITIONS = 10;
int WRITES_PER_TEST = 1'000;
int KEYS_TO_GENERATE = WRITES_PER_TEST;
int VALUES_TO_GENERATE = KEYS_TO_GENERATE;

int THREADS_MIN = 1;
int THREADS_MAX = std::thread::hardware_concurrency();
int THREAD_MULTIPLIER = 8;

class BenchmarkState {
 public:
  BenchmarkState(size_t value_size, int events_inflight = -1)
	  : pool_(std::make_unique<Pool>(POOL_LABEL)), value_size_(value_size),
		keys_(KEYS_TO_GENERATE), values_(VALUES_TO_GENERATE) {
	// Initialise keys to random strings
	for (auto& key : keys_) { key = std::to_string(rand()); }
	// Initialise values with given size
	for (auto& value : values_) { value.assign(value_size, 'A'); }

	// Generate unique container name
	container_name_ =
		"benchmark_container" + std::to_string(container_counter++);
	container_ = pool_->add_container(container_name_);
	key_value_store_ = container_->create_kv_object();

	// Support both running with event queue and without it
	if (events_inflight > 0) {
	  event_queue_ = std::make_unique<EventQueue>(events_inflight);
	} else {
	  event_queue_ = nullptr;
	}
  }
  const char* get_key(int i) const { return keys_[i % keys_.size()].c_str(); }
  size_t get_keys_count() const { return keys_.size(); }
  size_t get_values_count() const { return values_.size(); }
  const char* get_value(int i) const {
	return values_[i % values_.size()].data();
  }
  size_t get_value_size() const { return value_size_; }

  KeyValuePtr& get_kv_store() { return key_value_store_; }

  daos_event_t* get_event() {
	if (event_queue_) {
	  return event_queue_->get_event();
	}
	return NULL;
  }

  void wait_events() {
	if (event_queue_) {
	  event_queue_->wait();
	}
  }

  ~BenchmarkState() { pool_->remove_container(container_name_); }

 private:
  static int container_counter;
  std::string container_name_;

  std::unique_ptr<Pool> pool_;
  ContainerPtr container_;
  KeyValuePtr key_value_store_;
  std::unique_ptr<EventQueue> event_queue_;

  size_t value_size_;
  std::vector<std::string> keys_;
  std::vector<std::vector<char>> values_;
};

int BenchmarkState::container_counter = 0;

static void write_event_blocking(benchmark::State& state) {
  BenchmarkState bstate(state.range(0));
  int i = 0;
  for (auto _ : state) {
	i++;
	bstate.get_kv_store()->write_raw(bstate.get_key(i), bstate.get_value(i),
									 bstate.get_value_size());
  }
}

static void creating_events_kv_sync(benchmark::State& state) {
  BenchmarkState bstate(state.range(0));
  for (auto _ : state) {
	for (int i = 0; i < WRITES_PER_TEST; i++) {
	  bstate.get_kv_store()->write_raw(bstate.get_key(i), bstate.get_value(i),
									   bstate.get_value_size());
	}
  }
}

static void creating_events_kv_async(benchmark::State& state) {
  BenchmarkState bstate(state.range(0), state.range(1));
  for (auto _ : state) {
	for (int i = 0; i < WRITES_PER_TEST; i++) {
	  bstate.get_kv_store()->write_raw(bstate.get_key(i), bstate.get_value(i),
									   bstate.get_value_size(),
									   bstate.get_event());
	}
	bstate.wait_events();
  }
}

static void creating_events_array(benchmark::State& state) {
  BenchmarkState bstate(state.range(0));
  Pool pool(POOL_LABEL);
  std::string container_name = "benchmark_container";
  auto container = pool.add_container(container_name);
  auto array_store = container->create_array();
  for (auto _ : state) {
	for (int i = 0; i < WRITES_PER_TEST; i++) {
	  // FIXME: Casting away const
	  array_store->write_raw(i % bstate.get_keys_count(),
							 (char*)bstate.get_value(i), NULL);
	}
  }
  pool.remove_container(container_name);
}

void send_requests_sync(std::atomic_int32_t& sent_requests,
						BenchmarkState& bstate) {
  int i = 0;
  while (sent_requests.fetch_sub(1) > 1) {
	bstate.get_kv_store()->write_raw(bstate.get_key(i), bstate.get_value(i),
									 bstate.get_value_size());
	i++;
  }
}

void send_requests_async(std::atomic_int32_t& sent_requests,
						 BenchmarkState& bstate) {
  int i = 0;
  while (sent_requests.fetch_sub(1) > 1) {
	bstate.get_kv_store()->write_raw(bstate.get_key(i), bstate.get_value(i),
									 bstate.get_value_size(),
									 bstate.get_event());
	i++;
  }
  bstate.wait_events();
}

static void creating_events_multithreaded_single_container(
	benchmark::State& state) {
  std::atomic_int32_t sent_requests = WRITES_PER_TEST;
  int number_of_threads = state.range(2);
  auto bstate = std::make_unique<BenchmarkState>(state.range(0));
  for (auto _ : state) {
	std::vector<std::thread> threads;
	// for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	//   threads.emplace_back(send_requests_sync, std::ref(sent_requests),
	// 					   std::ref(bstate));
	// }
	for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	  threads[thread_n].join();
	}
  }
}

static void creating_events_multithreaded_single_container_async(
	benchmark::State& state) {
  std::atomic_int32_t sent_requests = WRITES_PER_TEST;
  int number_of_threads = state.range(2);
  auto bstate = std::make_unique<BenchmarkState>(state.range(0), state.range(1));
  for (auto _ : state) {
	std::vector<std::thread> threads;
	// for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	//   threads.emplace_back(send_requests_async, std::ref(sent_requests),
	// 					   std::ref(bstate));
	// }
	for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	  threads[thread_n].join();
	}
  }
}

static void creating_events_multitreaded_multiple_containers(
	benchmark::State& state) {
  std::atomic_int32_t sent_requests = WRITES_PER_TEST;
  int number_of_threads = state.range(2);

  std::vector<std::unique_ptr<BenchmarkState>> states;
  for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	states.emplace_back(std::make_unique<BenchmarkState>(state.range(0)));
  }

  for (auto _ : state) {
	std::vector<std::thread> threads;
	for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	  auto& bstate = *states[thread_n];
	  threads.emplace_back(send_requests_async, std::ref(sent_requests),
						   std::ref(bstate));
	}
	for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	  threads[thread_n].join();
	}
  }
}

static void creating_events_multitreaded_async(benchmark::State& state) {
  std::atomic_int32_t sent_requests = WRITES_PER_TEST;
  int number_of_threads = state.range(2);

  std::vector<std::unique_ptr<BenchmarkState>> states;
  for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	states.emplace_back(std::make_unique<BenchmarkState>(state.range(0)));
  }

  for (auto _ : state) {
	std::vector<std::thread> threads;
	for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	  auto& bstate = *states[thread_n];
	  threads.emplace_back(send_requests_async, std::ref(sent_requests),
						   std::ref(bstate));
	}
	for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	  threads[thread_n].join();
	}
  }
}

BENCHMARK(write_event_blocking)
	->ArgsProduct({benchmark::CreateDenseRange(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE,
											   CHUNK_SIZE_STEP),// Chunk size
				   UNUSED_RANGE, UNUSED_RANGE})
	->Repetitions(REPETITIONS)
	->Setup([](const benchmark::State&) { daos_init(); })
	->Teardown([](const benchmark::State&) { daos_fini(); });

BENCHMARK(creating_events_kv_sync)
	->ArgsProduct({benchmark::CreateDenseRange(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE,
											   CHUNK_SIZE_STEP),// Chunk size
				   UNUSED_RANGE, UNUSED_RANGE})
	->Repetitions(REPETITIONS)
	->Setup([](const benchmark::State&) { daos_init(); })
	->Teardown([](const benchmark::State&) { daos_fini(); });

BENCHMARK(creating_events_kv_async)
	->ArgsProduct(
		{benchmark::CreateDenseRange(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE,
									 CHUNK_SIZE_STEP),// Chunk size
		 benchmark::CreateDenseRange(INFLIGH_EVENTS_MIN, INFLIGH_EVENTS_MAX,
									 INFLIGH_EVENTS_STEP),// Inflight events
		 UNUSED_RANGE})
	->Repetitions(REPETITIONS)
	->Setup([](const benchmark::State&) { daos_init(); })
	->Teardown([](const benchmark::State&) { daos_fini(); });

BENCHMARK(creating_events_multitreaded)
	->ArgsProduct({benchmark::CreateDenseRange(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE,
											   CHUNK_SIZE_STEP),// Chunk size
				   UNUSED_RANGE,
				   benchmark::CreateRange(THREADS_MIN, THREADS_MAX,
										  THREAD_MULTIPLIER)})// Used cores
	->Repetitions(REPETITIONS)
	->Setup([](const benchmark::State&) { daos_init(); })
	->Teardown([](const benchmark::State&) { daos_fini(); });

BENCHMARK(creating_events_multitreaded_async)
	->ArgsProduct(
		{benchmark::CreateDenseRange(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE,
									 CHUNK_SIZE_STEP),// Chunk size
		 benchmark::CreateDenseRange(INFLIGH_EVENTS_MIN, INFLIGH_EVENTS_MAX,
									 INFLIGH_EVENTS_STEP),// Inflight events
		 benchmark::CreateRange(THREADS_MIN, THREADS_MAX, THREAD_MULTIPLIER)})
	->Repetitions(REPETITIONS)
	->Setup([](const benchmark::State&) { daos_init(); })
	->Teardown([](const benchmark::State&) { daos_fini(); });

BENCHMARK(creating_events_array)
	->ArgsProduct({benchmark::CreateDenseRange(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE,
											   CHUNK_SIZE_STEP),// Chunk size
				   UNUSED_RANGE, UNUSED_RANGE})
	->Repetitions(REPETITIONS)
	->Setup([](const benchmark::State&) { daos_init(); })
	->Teardown([](const benchmark::State&) { daos_fini(); });

BENCHMARK_MAIN();
