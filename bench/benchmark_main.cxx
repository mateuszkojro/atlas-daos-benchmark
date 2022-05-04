#include "DAOSZipkinLog.h"
#include "EventQueue.h"
#include "MockPool.h"
#include "Pool.h"
#include "UUID.h"
#include <benchmark/benchmark.h>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

std::string POOL_LABEL = "mkojro";
UUID POOL_UUID("3cb8ac1e-f0c0-4aa1-8cd2-af72b5e44c17");
int MIN_CHUNK_SIZE = 1024;
int MAX_CHUNK_SIZE = 10 * 1024;
int CHUNK_SIZE_STEP = 256;
int REPETITIONS = 10;
int WRITES_PER_TEST = 1000;
int KEYS_TO_GENERATE = WRITES_PER_TEST;
int VALUES_TO_GENERATE = KEYS_TO_GENERATE;

static void creating_events_kv_sync(benchmark::State& state) {
  size_t generated_values = VALUES_TO_GENERATE;
  size_t generated_keys = KEYS_TO_GENERATE;

  size_t value_size = state.range(0);

  std::vector<std::string> keys(generated_keys);
  std::vector<std::vector<char>> values(generated_values);

  for (auto& key : keys) { key = std::to_string(rand()); }

  for (auto& value : values) { value.assign(value_size, 'A'); }

  Pool pool(POOL_LABEL);
  auto container = pool.add_container("benchmark_container");
  auto key_value_store = container->create_kv_object();
  for (auto _ : state) {
	for (int i = 0; i < WRITES_PER_TEST; i++) {
	  key_value_store->write_raw(keys[i % generated_keys].c_str(),
								 values[i % generated_values].data(),
								 value_size);
	  benchmark::DoNotOptimize(i);
	}
  }
  pool.remove_container(container);
}

static void creating_events_kv_async(benchmark::State& state) {
  size_t generated_values = VALUES_TO_GENERATE;
  size_t generated_keys = KEYS_TO_GENERATE;

  size_t value_size = state.range(0);

  std::vector<std::string> keys(generated_keys);
  std::vector<std::vector<char>> values(generated_values);

  for (auto& key : keys) { key = std::to_string(rand()); }

  for (auto& value : values) { value.assign(value_size, 'A'); }

  Pool pool(POOL_LABEL);
  EventQueue eq(state.range(1));
  auto container = pool.add_container("benchmark_container");
  auto key_value_store = container->create_kv_object();
  for (auto _ : state) {
	for (int i = 0; i < WRITES_PER_TEST; i++) {
	  key_value_store->write_raw(keys[i % generated_keys].c_str(),
								 values[i % generated_values].data(),
								 value_size, eq.get_event());
	  benchmark::DoNotOptimize(i);
	}
	eq.wait();
  }
  pool.remove_container(container);
}

static void creating_events_array(benchmark::State& state) {
  size_t generated_values = VALUES_TO_GENERATE;
  size_t generated_keys = KEYS_TO_GENERATE;

  size_t value_size = state.range(0);

  std::vector<std::string> keys(generated_keys);
  std::vector<std::vector<char>> values(generated_values);

  for (auto& key : keys) { key = std::to_string(rand()); }

  for (auto& value : values) { value.assign(value_size, 'A'); }

  Pool pool(POOL_LABEL);
  auto container = pool.add_container("benchmark_container");
  auto array_store = container->create_array();
  for (auto _ : state) {
	for (int i = 0; i < WRITES_PER_TEST; i++) {
	  array_store->write_raw(i % generated_keys,
							 values[i % generated_values].data(), NULL);

	  benchmark::DoNotOptimize(i);
	}
  }
  pool.remove_container(container);
}

BENCHMARK(creating_events_kv_sync)
	->DenseRange(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE, CHUNK_SIZE_STEP)
	->Repetitions(REPETITIONS)
	->Setup([](const benchmark::State&) { daos_init(); })
	->Teardown([](const benchmark::State&) { daos_fini(); });

BENCHMARK(creating_events_kv_async)
	->ArgsProduct({benchmark::CreateDenseRange(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE,
											   CHUNK_SIZE_STEP),// Chunk size
				   benchmark::CreateDenseRange(1, 300, 25)})// Inflight events
	->Repetitions(REPETITIONS)
	->Setup([](const benchmark::State&) { daos_init(); })
	->Teardown([](const benchmark::State&) { daos_fini(); });

BENCHMARK(creating_events_array)
	->DenseRange(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE, CHUNK_SIZE_STEP)
	->Repetitions(REPETITIONS)
	->Setup([](const benchmark::State&) { daos_init(); })
	->Teardown([](const benchmark::State&) { daos_fini(); });

BENCHMARK_MAIN();
