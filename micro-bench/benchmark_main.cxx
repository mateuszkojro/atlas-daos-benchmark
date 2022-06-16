#include "EventQueue.h"
#include "MockPool.h"
#include "Pool.h"
#include "UUID.h"
#include "daos_types.h"
#include "interfaces.h"
#include <algorithm>
#include <atomic>
#include <benchmark/benchmark.h>
#include <bits/types/time_t.h>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <sys/resource.h>
#include <thread>
#include <vector>

#define UNUSED_RANGE benchmark::CreateDenseRange(1, 1, 1)
#define BENCHMARK_POOLING

std::string POOL_LABEL = "mkojro";
// UUID POOL_UUID("3cb8ac1e-f0c0-4aa1-8cd2-af72b5e44c17");
int MIN_CHUNK_SIZE = 1024;
int MAX_CHUNK_SIZE = 10 * 1024;
int CHUNK_SIZE_STEP = 1024;
int INFLIGH_EVENTS_MIN = 1;
int INFLIGH_EVENTS_MAX = 200;
int INFLIGH_EVENTS_STEP = 25;

int REPETITIONS = 2;
int REPETITIONS_PER_TEST = 1'000;
int KEYS_TO_GENERATE = REPETITIONS_PER_TEST;
int VALUES_TO_GENERATE = KEYS_TO_GENERATE;

auto KV_ASYNC_RNAGE = {
	benchmark::CreateDenseRange(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE,
								CHUNK_SIZE_STEP),// Chunk size
	benchmark::CreateRange(INFLIGH_EVENTS_MIN, INFLIGH_EVENTS_MAX,
						   4),// Inflight events
	UNUSED_RANGE};

int THREADS_MIN = 1;
int THREADS_MAX = std::thread::hardware_concurrency();
int THREAD_MULTIPLIER = 4;

int try_set_open_fd_soft_limit(unsigned long no_fd) {
  rlimit limit = {};
  int err = 0;
  err = getrlimit(RLIMIT_NOFILE, &limit);
  if (err != 0) {
	goto error;
  }
  limit.rlim_cur = std::clamp(no_fd, 0UL, limit.rlim_max);
  err = setrlimit(RLIMIT_NOFILE, &limit);
  if (err != 0) {
	goto error;
  }
  return err;

error:
  std::cout << "ERR: Limit could not be set: " << strerror(errno) << std::endl;
  return err;
}

class BenchmarkState {
 public:
  BenchmarkState(size_t value_size, int events_inflight = -1)
	  : pool_(std::make_unique<Pool>(POOL_LABEL)), value_size_(value_size),
		keys_(KEYS_TO_GENERATE), values_(VALUES_TO_GENERATE) {
	srand(seed);
	try_set_open_fd_soft_limit(200'000);
	// Initialise keys to random strings
	for (auto& key : keys_) { key = std::to_string(rand()); }
	// Initialise values with given size
	for (auto& value : values_) { value.assign(value_size, 'A'); }

	// Generate unique container name
	container_name_ = "benchmark_container"
					  + std::to_string(container_counter++)
					  + std::to_string(rand());
	container_ = pool_->add_container(container_name_);
	key_value_store_ = container_->create_kv_object();

	// Support both running with event queue and without it
	if (events_inflight > 0) {
	  event_queue_ = std::make_unique<EventQueue>(events_inflight);
	} else {
	  event_queue_ = nullptr;
	}
  }
  const std::vector<std::string>& get_keys() const { return keys_; }
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

  uint64_t wait_events() {
	if (event_queue_) {
	  auto start = std::chrono::high_resolution_clock::now();
	  event_queue_->wait();
	  auto end = std::chrono::high_resolution_clock::now();
	  return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
		  .count();
	}
	return 0;
  }

  ~BenchmarkState() { pool_->remove_container(container_name_); }

 private:
  static size_t container_counter;
  static size_t seed;
  std::string container_name_;

  std::unique_ptr<Pool> pool_;
  ContainerPtr container_;
  KeyValuePtr key_value_store_;
  std::unique_ptr<EventQueue> event_queue_;

  size_t value_size_;
  std::vector<std::string> keys_;
  std::vector<std::vector<char>> values_;
};

using BenchmarkStatePtr = std::unique_ptr<BenchmarkState>;
size_t BenchmarkState::container_counter = 0;
size_t BenchmarkState::seed = time(NULL);

static void write_event_blocking(benchmark::State& state) {
  BenchmarkState bstate(state.range(0));
  int i = 0;
  for (auto _ : state) {
	i++;
	bstate.get_kv_store()->write_raw(bstate.get_key(i), bstate.get_value(i),
									 bstate.get_value_size());
  }
}

static void creating_events_kv_async(benchmark::State& state) {
  BenchmarkState bstate(state.range(0), state.range(1));
  for (auto _ : state) {
	for (int i = 0; i < REPETITIONS_PER_TEST; i++) {
	  bstate.get_kv_store()->write_raw(bstate.get_key(i), bstate.get_value(i),
									   bstate.get_value_size(),
									   bstate.get_event());
	}
	state.counters["pooling_time_ns"] += bstate.wait_events();
  }
}

static void creating_events_array(benchmark::State& state) {
  BenchmarkState bstate(state.range(0));
  Pool pool(POOL_LABEL);
  std::string container_name = "benchmark_container";
  auto container = pool.add_container(container_name);
  auto array_store = container->create_array();
  for (auto _ : state) {
	for (int i = 0; i < REPETITIONS_PER_TEST; i++) {
	  // FIXME: Casting away const
	  array_store->write_raw(i % bstate.get_keys_count(),
							 (char*)bstate.get_value(i), NULL);
	}
  }
  pool.remove_container(container_name);
}

void do_write_(size_t requests_to_write, BenchmarkStatePtr& bstate) {
  int i = 0;
  while (requests_to_write--) {
	bstate->get_kv_store()->write_raw(bstate->get_key(i), bstate->get_value(i),
									  bstate->get_value_size());
	i++;
  }
}

static void creating_events_multithreaded_single_container(
	benchmark::State& state) {
  size_t requests_to_send = REPETITIONS_PER_TEST;
  int number_of_threads = state.range(2);
  auto bstate = std::make_unique<BenchmarkState>(state.range(0));
  for (auto _ : state) {
	std::vector<std::thread> threads;
	for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	  threads.emplace_back(do_write_, requests_to_send / number_of_threads,
						   std::ref(bstate));
	}
	for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	  threads[thread_n].join();
	}
  }
}

static void creating_events_multithreaded_single_container_async(
	benchmark::State& state) {

  size_t requests_to_send = REPETITIONS_PER_TEST;
  int number_of_threads = state.range(2);
  auto bstate =
	  std::make_unique<BenchmarkState>(state.range(0), state.range(1));
  for (auto _ : state) {
	std::vector<std::thread> threads;
	for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	  threads.emplace_back(do_write_, requests_to_send / number_of_threads,
						   std::ref(bstate));
	}
	for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	  threads[thread_n].join();
	}
  }
}

static void creating_events_multitreaded_multiple_containers(
	benchmark::State& state) {
  size_t requests_to_send = REPETITIONS_PER_TEST;
  int number_of_threads = state.range(2);

  std::vector<std::unique_ptr<BenchmarkState>> states;
  for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	states.emplace_back(std::make_unique<BenchmarkState>(state.range(0)));
  }

  for (auto _ : state) {
	std::vector<std::thread> threads;
	for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	  auto& bstate = states[thread_n];
	  threads.emplace_back(do_write_, requests_to_send / number_of_threads,
						   std::ref(bstate));
	}
	for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	  threads[thread_n].join();
	}
  }
}

static void creating_events_multitreaded_multiple_containers_async(
	benchmark::State& state) {
  size_t requests_to_send = REPETITIONS_PER_TEST;
  int number_of_threads = state.range(2);

  std::vector<std::unique_ptr<BenchmarkState>> states;
  for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	states.emplace_back(
		std::make_unique<BenchmarkState>(state.range(0), state.range(1)));
  }

  for (auto _ : state) {
	std::vector<std::thread> threads;
	for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	  auto& bstate = states[thread_n];
	  threads.emplace_back(do_write_, requests_to_send / number_of_threads,
						   std::ref(bstate));
	}
	for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	  threads[thread_n].join();
	}
  }
}

// TODO: now do the same thing with read benchmarks

void create_events(BenchmarkStatePtr& bstate) {
  for (const auto& key : bstate->get_keys()) {
	bstate->get_kv_store()->write_raw(key.c_str(), key.c_str(), key.size());
  }
}

void do_read(std::atomic_int32_t& sent_requests, BenchmarkStatePtr& bstate) {
  int i = 0;
  size_t size = bstate->get_value_size();
  char* buffer = new char[size];
  while (sent_requests.fetch_sub(1) > 1) {
	bstate->get_kv_store()->read_raw(bstate->get_key(i), buffer, size,
									 bstate->get_event());
	i++;
  }
  benchmark::DoNotOptimize(buffer);
  delete[] buffer;
}

static void reading_events_blocking(benchmark::State& state) {
  auto bstate = std::make_unique<BenchmarkState>(state.range(0));
  create_events(bstate);
  size_t size = bstate->get_value_size();
  char* buffer = new char[size];// TODO what size
  for (auto _ : state) {
	for (int i = 0; i < REPETITIONS_PER_TEST; i++) {
	  bstate->get_kv_store()->read_raw(bstate->get_key(i), buffer, size);
	}
  }
  delete[] buffer;
}
static void reading_events_async(benchmark::State& state) {
  auto bstate =
	  std::make_unique<BenchmarkState>(state.range(0), state.range(1));
  create_events(bstate);
  size_t size = bstate->get_value_size();
  char* buffer = new char[size];
  for (auto _ : state) {
	for (int i = 0; i < REPETITIONS_PER_TEST; i++) {
	  bstate->get_kv_store()->read_raw(bstate->get_key(i), buffer, size,
									   bstate->get_event());
	}
  }
  delete[] buffer;
}

static void reading_events_multithreaded_multiple_container_async(
	benchmark::State& state) {

  int number_of_threads = state.range(2);
  std::atomic_int32_t sent_requests = REPETITIONS_PER_TEST;

  std::vector<std::unique_ptr<BenchmarkState>> states;
  for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	states.emplace_back(
		std::make_unique<BenchmarkState>(state.range(0), state.range(1)));
	create_events(states.back());
  }
  for (auto _ : state) {
	std::vector<std::thread> threads;
	for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	  auto& bstate = states[thread_n];
	  threads.emplace_back(do_read, std::ref(sent_requests), std::ref(bstate));
	}
	for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	  threads[thread_n].join();
	}
  }
}
static void reading_events_multithreaded_multiple_container(
	benchmark::State& state) {
  int number_of_threads = state.range(2);
  std::atomic_int32_t sent_requests = REPETITIONS_PER_TEST;

  std::vector<std::unique_ptr<BenchmarkState>> states;
  for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	states.emplace_back(std::make_unique<BenchmarkState>(state.range(0)));
	create_events(states.back());
  }
  for (auto _ : state) {
	std::vector<std::thread> threads;
	for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	  auto& bstate = states[thread_n];
	  threads.emplace_back(do_read, std::ref(sent_requests), std::ref(bstate));
	}
	for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	  threads[thread_n].join();
	}
  }
}

static void read_events_multithreaded_single_container_async(
	benchmark::State& state) {
  std::atomic_int32_t sent_requests = REPETITIONS_PER_TEST;
  int number_of_threads = state.range(2);
  auto bstate =
	  std::make_unique<BenchmarkState>(state.range(0), state.range(1));
  for (auto _ : state) {
	std::vector<std::thread> threads;
	for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	  threads.emplace_back(do_read, std::ref(sent_requests), std::ref(bstate));
	}
	for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	  threads[thread_n].join();
	}
  }
}

static void reading_events_multithreaded_single_container(
	benchmark::State& state) {
  std::atomic_int32_t sent_requests = REPETITIONS_PER_TEST;
  int number_of_threads = state.range(2);
  auto bstate = std::make_unique<BenchmarkState>(state.range(0));
  for (auto _ : state) {
	std::vector<std::thread> threads;
	for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	  threads.emplace_back(do_read, std::ref(sent_requests), std::ref(bstate));
	}
	for (int thread_n = 0; thread_n < number_of_threads; thread_n++) {
	  threads[thread_n].join();
	}
  }
}

// TODO: Add reading from array benchmark as a limit
BENCHMARK(creating_events_array)
	->ArgsProduct({benchmark::CreateDenseRange(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE,
											   CHUNK_SIZE_STEP),// Chunk size
				   UNUSED_RANGE, UNUSED_RANGE})
	->Repetitions(REPETITIONS)
	->Setup([](const benchmark::State&) { daos_init(); })
	->Teardown([](const benchmark::State&) { daos_fini(); });

// WRITING BENCHMARKS

BENCHMARK(write_event_blocking)
	->ArgsProduct({benchmark::CreateDenseRange(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE,
											   CHUNK_SIZE_STEP),// Chunk size
				   UNUSED_RANGE, UNUSED_RANGE})
	->Repetitions(REPETITIONS)
	->Setup([](const benchmark::State&) { daos_init(); })
	->Teardown([](const benchmark::State&) { daos_fini(); });

BENCHMARK(creating_events_kv_async)
	->ArgsProduct(KV_ASYNC_RNAGE)
	->Repetitions(REPETITIONS)
	->Setup([](const benchmark::State&) { daos_init(); })
	->Teardown([](const benchmark::State&) { daos_fini(); });

BENCHMARK(creating_events_multitreaded_multiple_containers)
	->ArgsProduct({benchmark::CreateDenseRange(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE,
											   CHUNK_SIZE_STEP),// Chunk size
				   UNUSED_RANGE,
				   benchmark::CreateRange(THREADS_MIN, THREADS_MAX,
										  THREAD_MULTIPLIER)})// Used cores
	->Repetitions(REPETITIONS)
	->Setup([](const benchmark::State&) { daos_init(); })
	->Teardown([](const benchmark::State&) { daos_fini(); });

// BENCHMARK(creating_events_multitreaded_multiple_containers_async)
// 	->ArgsProduct(
// 		{benchmark::CreateDenseRange(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE,
// 									 CHUNK_SIZE_STEP),// Chunk size
// 		 benchmark::CreateDenseRange(INFLIGH_EVENTS_MIN, INFLIGH_EVENTS_MAX,
// 									 INFLIGH_EVENTS_STEP),// Inflight events
// 		 benchmark::CreateRange(THREADS_MIN, THREADS_MAX, THREAD_MULTIPLIER)})
// 	->Repetitions(REPETITIONS)
// 	->Setup([](const benchmark::State&) { daos_init(); })
// 	->Teardown([](const benchmark::State&) { daos_fini(); });

BENCHMARK(creating_events_multithreaded_single_container)
	->ArgsProduct({benchmark::CreateDenseRange(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE,
											   CHUNK_SIZE_STEP),// Chunk size
				   UNUSED_RANGE,
				   benchmark::CreateRange(THREADS_MIN, THREADS_MAX,
										  THREAD_MULTIPLIER)})// Used cores
	->Repetitions(REPETITIONS)
	->Setup([](const benchmark::State&) { daos_init(); })
	->Teardown([](const benchmark::State&) { daos_fini(); });

// READING BENCHMARKS

// BENCHMARK(reading_events_blocking)
// 	->ArgsProduct({benchmark::CreateDenseRange(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE,
// 											   CHUNK_SIZE_STEP),// Chunk size
// 				   UNUSED_RANGE, UNUSED_RANGE})
// 	->Repetitions(REPETITIONS)
// 	->Setup([](const benchmark::State&) { daos_init(); })
// 	->Teardown([](const benchmark::State&) { daos_fini(); });

// BENCHMARK(reading_events_async)
// 	->ArgsProduct(
// 		{benchmark::CreateDenseRange(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE,
// 									 CHUNK_SIZE_STEP),// Chunk size
// 		 benchmark::CreateDenseRange(INFLIGH_EVENTS_MIN, INFLIGH_EVENTS_MAX,
// 									 INFLIGH_EVENTS_STEP),// Inflight events
// 		 UNUSED_RANGE})
// 	->Repetitions(REPETITIONS)
// 	->Setup([](const benchmark::State&) { daos_init(); })
// 	->Teardown([](const benchmark::State&) { daos_fini(); });

// BENCHMARK(reading_events_multithreaded_multiple_container)
// 	->ArgsProduct({benchmark::CreateDenseRange(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE,
// 											   CHUNK_SIZE_STEP),// Chunk size
// 				   UNUSED_RANGE,
// 				   benchmark::CreateRange(THREADS_MIN, THREADS_MAX,
// 										  THREAD_MULTIPLIER)})// Used cores
// 	->Repetitions(REPETITIONS)
// 	->Setup([](const benchmark::State&) { daos_init(); })
// 	->Teardown([](const benchmark::State&) { daos_fini(); });

// // BENCHMARK(reading_events_multithreaded_multiple_container_async)
// // 	->ArgsProduct(
// // 		{benchmark::CreateDenseRange(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE,
// // 									 CHUNK_SIZE_STEP),// Chunk size
// // 		 benchmark::CreateDenseRange(INFLIGH_EVENTS_MIN, INFLIGH_EVENTS_MAX,
// // 									 INFLIGH_EVENTS_STEP),// Inflight events
// // 		 benchmark::CreateRange(THREADS_MIN, THREADS_MAX,
// THREAD_MULTIPLIER)})
// // 	->Repetitions(REPETITIONS)
// // 	->Setup([](const benchmark::State&) { daos_init(); })
// // 	->Teardown([](const benchmark::State&) { daos_fini(); });

// // // BENCHMARK(reading_events_multithreaded_single_container)
// // // 	->ArgsProduct({benchmark::CreateDenseRange(MIN_CHUNK_SIZE,
// MAX_CHUNK_SIZE,
// // // 											   CHUNK_SIZE_STEP),// Chunk
// size
// // // 				   UNUSED_RANGE,
// // // 				   benchmark::CreateRange(THREADS_MIN, THREADS_MAX,
// // // 										  THREAD_MULTIPLIER)})// Used
// cores
// // // 	->Repetitions(REPETITIONS)
// // // 	->Setup([](const benchmark::State&) { daos_init(); })
// // // 	->Teardown([](const benchmark::State&) { daos_fini(); });

// BENCHMARK(read_events_multithreaded_single_container_async)
// 	->ArgsProduct({benchmark::CreateDenseRange(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE,
// 											   CHUNK_SIZE_STEP),// Chunk size
// 				   UNUSED_RANGE,
// 				   benchmark::CreateRange(THREADS_MIN, THREADS_MAX,
// 										  THREAD_MULTIPLIER)})// Used cores
// 	->Repetitions(REPETITIONS)
// 	->Setup([](const benchmark::State&) { daos_init(); })
// 	->Teardown([](const benchmark::State&) { daos_fini(); });

// BENCHMARK(creating_events_multithreaded_single_container_async)
// 	->ArgsProduct(
// 		{benchmark::CreateDenseRange(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE,
// 									 CHUNK_SIZE_STEP),// Chunk size
// 		 benchmark::CreateDenseRange(INFLIGH_EVENTS_MIN, INFLIGH_EVENTS_MAX,
// 									 INFLIGH_EVENTS_STEP),// Inflight events
// 		 benchmark::CreateRange(THREADS_MIN, THREADS_MAX, THREAD_MULTIPLIER)})
// 	->Repetitions(REPETITIONS)
// 	->Setup([](const benchmark::State&) { daos_init(); })
// 	->Teardown([](const benchmark::State&) { daos_fini(); });

BENCHMARK_MAIN();