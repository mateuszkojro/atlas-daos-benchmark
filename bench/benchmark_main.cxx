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

std::string label = "mkojro";
UUID uuid("3cb8ac1e-f0c0-4aa1-8cd2-af72b5e44c17");

static void creating_events_kv_sync(benchmark::State& state) {
  size_t generated_values = 100;
  size_t generated_keys = 100;

  size_t value_size = state.range(0);

  std::vector<std::string> keys(generated_keys);
  std::vector<std::vector<char>> values(generated_values);

  for (auto& key : keys) { key = std::to_string(rand()); }

  for (auto& value : values) { value.assign(value_size, 'A'); }

  Pool pool(label);
  auto container = pool.add_container("benchmark_container");
  auto key_value_store = container->create_kv_object();
  for (auto _ : state) {
	for (int i = 0; i < 1000; i++) {
	  key_value_store->write_raw(keys[i % generated_keys].c_str(),
								 values[i % generated_values].data(),
								 value_size);
	  benchmark::DoNotOptimize(i);
	}
  }
  pool.remove_container(container);
}

static void creating_events_kv_async(benchmark::State& state) {
  size_t generated_values = 100;
  size_t generated_keys = 100;

  size_t value_size = state.range(0);

  std::vector<std::string> keys(generated_keys);
  std::vector<std::vector<char>> values(generated_values);

  for (auto& key : keys) { key = std::to_string(rand()); }

  for (auto& value : values) { value.assign(value_size, 'A'); }

  Pool pool(label);
  EventQueue eq(100);
  auto container = pool.add_container("benchmark_container");
  auto key_value_store = container->create_kv_object();
  for (auto _ : state) {
	for (int i = 0; i < 1000; i++) {
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
  size_t generated_values = 100;
  size_t generated_keys = 100;

  size_t value_size = state.range(0);

  std::vector<std::string> keys(generated_keys);
  std::vector<std::vector<char>> values(generated_values);

  for (auto& key : keys) { key = std::to_string(rand()); }

  for (auto& value : values) { value.assign(value_size, 'A'); }

  Pool pool(label);
  auto container = pool.add_container("benchmark_container");
  auto array_store = container->create_array();
  for (auto _ : state) {
	for (int i = 0; i < 1000; i++) {
	  array_store->write_raw(i % generated_keys,
							 values[i % generated_values].data(), NULL);

	  benchmark::DoNotOptimize(i);
	}
  }
  pool.remove_container(container);
}

BENCHMARK(creating_events_kv_sync)
	->Repetitions(10)
	->RangeMultiplier(2)
	->Range(1024, 64 * 1024)
	->RangeMultiplier(2)
	->Range(5 * 1024, 64 * 1024)
	->Range(10 * 1024, 1024 * 1024)
	->Setup([](const benchmark::State&) { daos_init(); })
	->Teardown([](const benchmark::State&) { daos_fini(); });

BENCHMARK(creating_events_kv_async)
  ->Repetitions(10)
	->RangeMultiplier(2)
	->Range(1024, 64 * 1024)
	->RangeMultiplier(2)
	->Range(5 * 1024, 64 * 1024)
	->Range(10 * 1024, 1024 * 1024)
	->Setup([](const benchmark::State&) { daos_init(); })
	->Teardown([](const benchmark::State&) { daos_fini(); });

BENCHMARK(creating_events_array)
  ->Repetitions(10)
	->RangeMultiplier(2)
	->Range(1024, 64 * 1024)
	->RangeMultiplier(2)
	->Range(5 * 1024, 64 * 1024)
	->Range(10 * 1024, 1024 * 1024)
	->Setup([](const benchmark::State&) { daos_init(); })
	->Teardown([](const benchmark::State&) { daos_fini(); });

BENCHMARK_MAIN();
