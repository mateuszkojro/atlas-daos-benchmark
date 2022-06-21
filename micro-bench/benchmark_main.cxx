#include "EventQueue.h"
#include "MockPool.h"
#include "Pool.h"
#include "UUID.h"
#include "backtrace.h"
#include "daos.h"
#include "daos_types.h"
#include "interfaces.h"
#include "toml.h"
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
#include <stdexcept>
#include <string>
#include <sys/resource.h>
#include <thread>
#include <vector>

#define UNUSED_RANGE benchmark::CreateDenseRange(1, 1, 1)
#define BENCHMARK_POOLING

const size_t REPETITIONS = 1;
const size_t REPETITIONS_PER_TEST = 1'000;
int KEYS_TO_GENERATE = REPETITIONS_PER_TEST;
int VALUES_TO_GENERATE = KEYS_TO_GENERATE;

std::string POOL_LABEL = "mkojro";

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

#define bench_print(format_string) printf("[bench] " format_string "\n")

#define bench_printf(format_string, ...)                                       \
  fprintf(stderr, "[bench] " format_string "\n", __VA_ARGS__)

#define bench_check(condition, name)                                           \
  do {                                                                         \
	fprintf(stderr, "[bench] %-64s\t", name);                                  \
	if (condition) {                                                           \
	  fprintf(stderr, "\033[1;92mOK\033[0m\n");                                \
	} else {                                                                   \
	  fprintf(stderr, "\033[1;31mERR\033[0m\n");                               \
	}                                                                          \
  } while (false)

class Config {
 public:
  enum {
	NONE = 0,
	WITH_CHNUK_SIZE = 1 << 0,
	WITH_THREADS = 1 << 1,
	WITH_EVENTS = 1 << 2
  };

  //   static void parse_config(std::string path) { instance_ = new
  //   Config(path); }
  static Config* instance() {
	if (instance_ == nullptr) {
	  instance_ = new Config("micro_benchmark_config.toml");
	}
	return instance_;
  }

  std::vector<int64_t> get_range_for_variable(std::string variable) {
	std::string range_type =
		configuration_file_[variable]["range_type"].value_or("log");
	auto min = configuration_file_[variable]["min"].value_or(1);
	auto max = configuration_file_[variable]["max"].value_or(1);
	auto step = configuration_file_[variable]["step"].value_or(2);
	std::vector<int64_t> range;
	if (range_type == "log") {
	  range = benchmark::CreateRange(min, max, step);
	} else if (range_type == "dense") {
	  range = benchmark::CreateDenseRange(min, max, step);
	} else {
	  throw std::runtime_error(
		  "Bad options for range_type avaliable: 'dense', 'log'");
	}
	return range;
  }

  std::vector<std::vector<int64_t>> get_range(int options) {
	std::vector<std::vector<int64_t>> ranges(3, UNUSED_RANGE);
	bool with_chunk_size_set = options & Config::WITH_CHNUK_SIZE;
	bool with_inflight_set = options & Config::WITH_EVENTS;
	bool with_threads_set = options & Config::WITH_THREADS;
	if (with_chunk_size_set) {
	  ranges[0] = get_range_for_variable("chunk_size");
	}
	if (with_inflight_set) {
	  ranges[1] = get_range_for_variable("inflight_events");
	}
	if (with_threads_set) {
	  ranges[2] = get_range_for_variable("threads");
	}
	return ranges;
  }

  static void close() { delete instance_; }

 private:
  void configure_system() {
	// Initialisation
	bench_print("Seeding random");
	srand(time(NULL));
	bench_check(try_set_open_fd_soft_limit(200'000) == 0,
				"Trying to increase fd count limit");
	bench_check(bt_init() == 0, "Register backtrace handlers");
  }
  Config(std::string path) {
	configure_system();
	configuration_file_ = toml::parse_file(path);
  }

  static Config* instance_;
  toml::parse_result configuration_file_;
};
Config* Config::instance_ = nullptr;

class BenchmarkState {
 public:
  BenchmarkState(size_t value_size, benchmark::State& state,
				 int events_inflight = -1)
	  : state_(state), pool_(std::make_unique<Pool>(POOL_LABEL)),
		value_size_(value_size), keys_(KEYS_TO_GENERATE),
		values_(VALUES_TO_GENERATE) {

	config_ = Config::instance();

	state_.counters["pooling_time_ns"] =
		benchmark::Counter(0, benchmark::Counter::kAvgIterations);

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
	  event_queue_->wait();
	  size_t waiting_time = event_queue_->waiting_time_;
	  event_queue_->waiting_time_.store(0);
	  state_.counters["pooling_time_ns"] += waiting_time;
	  return waiting_time;
	}
	return 0;
  }

  ~BenchmarkState() { pool_->remove_container(container_name_); }

 private:
  benchmark::State& state_;
  Config* config_ = nullptr;
  static size_t container_counter;
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

static void baseline_BenchmarkState_usage(benchmark::State& state) {
  BenchmarkState bstate(state.range(0), state);
  int i = 0;
  for (auto _ : state) {
	const char* key = bstate.get_key(i);
	const char* value = bstate.get_value(i);
	size_t size = bstate.get_value_size();
	KeyValuePtr& kv = bstate.get_kv_store();
	benchmark::DoNotOptimize(key);
	benchmark::DoNotOptimize(value);
	benchmark::DoNotOptimize(size);
	benchmark::DoNotOptimize(kv);
  }
}

static void write_event_blocking(benchmark::State& state) {
  BenchmarkState bstate(state.range(0), state);
  for (auto _ : state) {
	for (int i = 0; i < REPETITIONS_PER_TEST; i++) {
	  bstate.get_kv_store()->write_raw(bstate.get_key(i), bstate.get_value(i),
									   bstate.get_value_size());
	}
  }
}

static void creating_events_kv_async(benchmark::State& state) {
  BenchmarkState bstate(state.range(0), state, state.range(1));
  for (auto _ : state) {
	for (int i = 0; i < REPETITIONS_PER_TEST; i++) {
	  bstate.get_kv_store()->write_raw(bstate.get_key(i), bstate.get_value(i),
									   bstate.get_value_size(),
									   bstate.get_event());
	}
	bstate.wait_events();
  }
}

static void creating_events_array(benchmark::State& state) {
  BenchmarkState bstate(state.range(0), state);
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
  for (int i = 0; i < requests_to_write; i++) {
	bstate->get_kv_store()->write_raw(bstate->get_key(i), bstate->get_value(i),
									  bstate->get_value_size(),
									  bstate->get_event());
  }
  bstate->wait_events();
}

static void creating_events_multithreaded_single_container(
	benchmark::State& state) {
  size_t requests_to_send = REPETITIONS_PER_TEST;
  int number_of_threads = state.range(2);
  auto bstate = std::make_unique<BenchmarkState>(state.range(0), state);
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
	  std::make_unique<BenchmarkState>(state.range(0), state, state.range(1));
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
	states.emplace_back(
		std::make_unique<BenchmarkState>(state.range(0), state));
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
	states.emplace_back(std::make_unique<BenchmarkState>(state.range(0), state,
														 state.range(1)));
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


BENCHMARK(baseline_BenchmarkState_usage)
	->ArgsProduct(Config::instance()->get_range(Config::NONE))
	->Repetitions(REPETITIONS);

BENCHMARK(creating_events_array)
	->ArgsProduct(Config::instance()->get_range(Config::WITH_CHNUK_SIZE))
	->Repetitions(REPETITIONS);

BENCHMARK(write_event_blocking)
	->ArgsProduct(Config::instance()->get_range(Config::WITH_CHNUK_SIZE))
	->Repetitions(REPETITIONS);

BENCHMARK(creating_events_kv_async)
	->ArgsProduct(Config::instance()->get_range(Config::WITH_CHNUK_SIZE
												| Config::WITH_EVENTS))
	->Repetitions(REPETITIONS);

BENCHMARK(creating_events_multitreaded_multiple_containers)
	->ArgsProduct(Config::instance()->get_range(Config::WITH_CHNUK_SIZE
												| Config::WITH_THREADS))
	->Repetitions(REPETITIONS);

BENCHMARK(creating_events_multitreaded_multiple_containers_async)
	->ArgsProduct(Config::instance()->get_range(Config::WITH_CHNUK_SIZE
												| Config::WITH_EVENTS
												| Config::WITH_THREADS))
	->Repetitions(REPETITIONS);

BENCHMARK(creating_events_multithreaded_single_container)
	->ArgsProduct(Config::instance()->get_range(Config::WITH_CHNUK_SIZE
												| Config::WITH_EVENTS
												| Config::WITH_THREADS))
	->Repetitions(REPETITIONS);

BENCHMARK(creating_events_multithreaded_single_container_async)
	->ArgsProduct(Config::instance()->get_range(Config::WITH_CHNUK_SIZE
												| Config::WITH_EVENTS
												| Config::WITH_THREADS))
	->Repetitions(REPETITIONS);

int main(int argc, char** argv) {
  benchmark::Initialize(&argc, argv);
  if (benchmark::ReportUnrecognizedArguments(argc, argv))
	return 1;
  daos_init();
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
  daos_fini();
  Config::close();
  return 0;
}
