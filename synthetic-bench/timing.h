#ifndef MK_TIMING_H
#define MK_TIMING_H

#include "EventQueue.h"
#include "KeyValue.h"
#include "Pool.h"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct ArrayConfig
{
  size_t chunk_size;
  size_t cell_size;
};

struct KeyValueConfig
{
  // Hash type
};

struct TestConfig
{
  static TestConfig generate_array_config(bool with_event_queue,
										  size_t event_fragment_size,
										  size_t inflight_events,
										  size_t fragments_to_safe,
										  size_t chunk_size, size_t cell_size) {
	TestConfig test_config;
	test_config.using_event_queue = with_event_queue;
	test_config.event_fragment_size = event_fragment_size;
	test_config.inflight_events = inflight_events;
	test_config.fragments_to_safe = fragments_to_safe;

	test_config.config_type = TestConfig::ARRAY_CONFIG;
	ArrayConfig array_config;
	array_config.cell_size = cell_size;
	array_config.chunk_size = chunk_size;
	test_config.get.array_config = array_config;

	return test_config;
  }

  static TestConfig generate_key_value_config(bool with_event_queue,
											  size_t event_fragment_size,
											  size_t inflight_events,
											  size_t fragments_to_safe) {
	TestConfig test_config;
	test_config.using_event_queue = with_event_queue;
	test_config.event_fragment_size = event_fragment_size;
	test_config.inflight_events = inflight_events;
	test_config.fragments_to_safe = fragments_to_safe;
	test_config.config_type = TestConfig::KEY_VALUE_CONFIG;

	KeyValueConfig key_value_config;
	test_config.get.key_value_config = key_value_config;

	return test_config;
  }

  // std::string test_name;
  bool using_event_queue;
  size_t fragments_to_safe;
  size_t inflight_events;
  size_t event_fragment_size;
  enum
  {
	ARRAY_CONFIG,
	KEY_VALUE_CONFIG
  } config_type;

  union {
	ArrayConfig array_config;
	KeyValueConfig key_value_config;
  } get;
};

struct TimingInfo
{
  TimingInfo(std::string name, std::vector<std::chrono::microseconds> timings)
	  : name(name), timings(timings) {}
  std::string name;
  std::vector<std::chrono::microseconds> timings;
};

class Harness {

 public:
  Harness(std::vector<TestConfig>& configurations_to_test, Pool& pool)
	  : configurations_to_test_(configurations_to_test), pool_(pool) {}

  void execute_with_config(const TestConfig& config) {
	auto container = pool_.add_container();
	size_t array_index = 0;
	const char* key = "";
	std::vector<uint8_t> buffer(config.event_fragment_size);
	EventQueue event_queue(config.inflight_events);

	try {
	  switch (config.config_type) {
		case TestConfig::ARRAY_CONFIG: {
		  auto array = container->create_array();
		  for (size_t fragment = 0; fragment < config.fragments_to_safe;
			   fragment++) {
			array->write_raw(array_index, (char*)buffer.data(),
							 config.using_event_queue ? event_queue.get_event()
													  : NULL);
		  }
		  break;
		}
		case TestConfig::KEY_VALUE_CONFIG: {
		  auto key_value = container->create_kv_object();
		  for (size_t fragment = 0; fragment < config.fragments_to_safe;
			   fragment++) {
			key_value->write_raw(
				key, (char*)buffer.data(), buffer.size(),
				config.using_event_queue ? event_queue.get_event() : NULL);
		  }
		  break;
		}
	  }

	  if (config.using_event_queue) {
		event_queue.wait();
	  }
	  pool_.remove_container(container);
	} catch (std::exception& e) { pool_.remove_container(container); }
  }

  std::vector<TimingInfo> measure(size_t iterations_per_config = 10) {
	using std::chrono::duration_cast;
	using std::chrono::high_resolution_clock;
	using std::chrono::microseconds;

	std::vector<TimingInfo> results;
	results.reserve(configurations_to_test_.size());

	size_t config_number = 0;
	for (auto& config : configurations_to_test_) {
	  config_number++;
	  std::vector<microseconds> results_for_config;
	  results_for_config.reserve(iterations_per_config);
	  std::clog << "Config " << config_number << "/"
				<< configurations_to_test_.size() << " ... ";
	  for (size_t iteration = 0; iteration < iterations_per_config;
		   iteration++) {
		auto start = high_resolution_clock::now();
		execute_with_config(config);
		auto end = high_resolution_clock::now();
		results_for_config.push_back(duration_cast<microseconds>(end - start));
	  }
	  std::clog << "done" << std::endl;
	  results.push_back(TimingInfo("name", results_for_config));
	}

	return results;
  }

 private:
  std::vector<TestConfig>& configurations_to_test_;
  Pool& pool_;
};

static TimingInfo measure_time(std::function<void(void)> function,
							   std::string name, std::size_t iterations) {
  using std::chrono::duration_cast;
  using std::chrono::high_resolution_clock;
  using std::chrono::microseconds;

  std::vector<microseconds> timings;
  timings.reserve(iterations);
  for (size_t i = 0; i < iterations; i++) {
	auto start = high_resolution_clock::now();
	function();
	auto stop = high_resolution_clock::now();
	timings.push_back(duration_cast<microseconds>(stop - start));
  }
  return TimingInfo(name, timings);
}

#endif// !MK_TIMING_H