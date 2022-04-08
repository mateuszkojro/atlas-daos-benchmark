#include "Pool.h"
#include <benchmark/benchmark.h>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

std::string label = "tank";

static void creating_events_kv(benchmark::State& state) {
  size_t generated_values = 100;
  size_t generated_keys = 100;

  size_t value_size = state.range(0);

  std::vector<std::string> keys(generated_keys);
  std::vector<std::vector<char>> values(generated_values);

  for (auto& key : keys) { key = std::to_string(rand()); }

  for (auto& value : values) { value.assign(value_size, 'A'); }

  Pool pool(label);
  auto container = pool.add_container();
  auto key_value_store = container->create_kv_object();
  int i = 0;
  for (auto _ : state) {
	key_value_store->write_raw(keys[i % generated_keys].c_str(),
				   values[i % generated_values].data(), value_size);
	i++;
  }
  pool.remove_container(container);
}
// Register the function as a benchmark
BENCHMARK(creating_events_kv)
	->Range(5*1024, 64*1024)
	->Setup([](const benchmark::State&) { daos_init(); })
	->Teardown([](const benchmark::State&) { daos_fini(); });

BENCHMARK_MAIN();
