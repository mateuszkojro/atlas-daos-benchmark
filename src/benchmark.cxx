#include "Array.h"
#include "Container.h"
#include "EventQueue.h"
#include "Pool.h"
#include "daos_event.h"
#include "daos_types.h"
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <functional>
#include <iostream>
#include <ratio>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "timing.h"

#define KB(x) 1024 * x
#define MB(x) 1024 * 1024 * x

using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::microseconds;

const int number_ok_events_to_save = 1000;
const int event_size = CELL_SIZE;// MB(5);

void test_kv(ContainerPtr& container, EventQueue& event_queue) {
  auto key_value = container->create_kv_object();

  auto start = high_resolution_clock::now();
  for (int i = 0; i < number_ok_events_to_save; i++) {
	key_value->write_raw(std::to_string(i).c_str(), "bb", event_size,
						 event_queue.get_event());
  }

  event_queue.wait();
  auto end = high_resolution_clock::now();
  std::cout << "Using event queue:\t"
			<< duration_cast<microseconds>(end - start).count() << "us\n";

  start = high_resolution_clock::now();
  for (int i = 0; i < number_ok_events_to_save; i++) {
	key_value->write_raw(std::to_string(i + number_ok_events_to_save).c_str(),
						 "bb", event_size, NULL);
  }
  end = high_resolution_clock::now();
  std::cout << "Without event queue:\t"
			<< duration_cast<microseconds>(end - start).count() << "us\n";
}

void test_array(ContainerPtr& container, EventQueue& event_queue) {
  auto array = container->create_array();

  char* buffer = new char[CELL_SIZE];
  buffer[0] = 'A';

  auto start = high_resolution_clock::now();
  for (int i = 0; i < number_ok_events_to_save; i++) {
	array->write_raw(i, buffer);
  }
  auto end = high_resolution_clock::now();
  std::cout << "Array w/o eq:\t"
			<< duration_cast<microseconds>(end - start).count() << "us"
			<< std::endl;

  start = high_resolution_clock::now();
  for (int i = 0; i < number_ok_events_to_save; i++) {
	array->write_raw(i, buffer, event_queue.get_event());
  }
  event_queue.wait();
  end = high_resolution_clock::now();
  std::cout << "Array using eq:\t"
			<< duration_cast<microseconds>(end - start).count() << "us"
			<< std::endl;

  // for(int i = 0 ; i < number_ok_events_to_save; i++){
  //   object.read_raw(i);
  // }
}

void simple_benchmark(Pool& pool) {
  auto kv_container = pool.add_container();
  auto array_container = pool.add_container();

  const int inflight = 1000;
  EventQueue event_queue(inflight);

  std::cout << "--- Testing kv store" << std::endl;
  try {
	test_kv(kv_container, event_queue);
	std::cout << "kv_test - succes!" << std::endl;
  } catch (std::exception& e) {
	std::cout << "Error occurred: " << e.what() << std::endl;
  } catch (...) { std::cout << "Unknown error occurred" << std::endl; }
  std::cout << std::endl << std::endl;

  std::cout << "--- Testing array store" << std::endl;
  try {
	test_array(array_container, event_queue);
	std::cout << "array_test - succes!" << std::endl;
  } catch (std::exception& e) {
	std::cout << "Error occurred: " << e.what() << std::endl;
  } catch (...) { std::cout << "Unknown error occurred" << std::endl; }
  pool.remove_container(kv_container);
  pool.remove_container(array_container);
}

const int FRAGMENTS_TO_SAVE = 10'000;

struct Context
{
  Context(Pool& pool, EventQueue& event_queue, std::vector<char>& data)
	  : pool(pool), event_queue(event_queue), data(data) {}
  Pool& pool;
  EventQueue& event_queue;
  std::vector<char>& data;
};

const char* gen_key() { return "expression"; }

std::vector<TimingInfo> all_timings;

void sweep_kv(const Context& context) {

  using KV_TYPE = int;
  std::vector<KV_TYPE> kv_type_settings;

  KeyValuePtr kv_store;
  for (auto kv_type : kv_type_settings) {

	(void)kv_type;

	std::vector<std::function<void(void)>> functions = {
		[&context, &kv_store]() {
		  kv_store->write_raw(gen_key(), context.data.data(),
							  context.data.size(),
							  context.event_queue.get_event());
		},
		[&context, &kv_store]() {
		  kv_store->write_raw(gen_key(), context.data.data(),
							  context.data.size(), NULL);
		},

	};

	for (auto function : functions) {
	  auto container = context.pool.add_container();
	  auto kv_store = container->create_kv_object();
	  for (int i = 0; i < FRAGMENTS_TO_SAVE; i++) {
		auto result = measure_time(function, "name", FRAGMENTS_TO_SAVE);
		all_timings.push_back(result);
		context.pool.remove_container(container);
	  }
	}
  }
}

void sweep_array(Context& context) {
  std::vector<size_t> chunk_size_settings;
  std::vector<size_t> cell_size_settings;

  ArrayPtr array_store;
  for (auto chunk_size : chunk_size_settings) {
	for (auto cell_size : cell_size_settings) {

	  (void)chunk_size;
	  (void)cell_size;

	  std::vector<std::function<void(void)>> functions = {
		  [&context, &array_store]() {
			// FIXME: Get index
			array_store->write_raw(1, context.data.data(), NULL);
		  }};

	  for (auto function : functions) {
		auto container = context.pool.add_container();
		auto kv_store = container->create_array();
		try {
		  for (int i = 0; i < FRAGMENTS_TO_SAVE; i++) {
			auto result = measure_time(function, "name", FRAGMENTS_TO_SAVE);
			all_timings.push_back(result);
			context.pool.remove_container(container);
		  }
		} catch (std::exception& e) {
		  std::cout << e.what();
		  context.pool.remove_container(container);
		}
	  }
	}
  }
}

void sweep_settings(Pool& pool) {
  std::vector<size_t> inflight_events_settings;
  std::vector<size_t> data_size_settings;

  for (auto inflight_events : inflight_events_settings) {
	EventQueue event_queue(inflight_events);
	for (auto data_size : data_size_settings) {
	  std::vector<char> data(data_size);

	  Context context(pool, event_queue, data);
	  sweep_kv(context);
	  sweep_array(context);
	}
  }
}

std::vector<TestConfig> generate_configurations() {
  const size_t EVENTS_INFLIGHT_MAX = 1000;
  const size_t EVENTS_INFLIGHT_MIM = 0;
  const size_t EVENTS_INFLIGHT_STEP = 100;

  const size_t FRAGMENTS_TO_SAVE_MAX = 1'000;
  const size_t FRAGMENTS_TO_SAVE_MIM = 100;
  const size_t FRAGMENTS_TO_SAVE_STEP = 100;

  const size_t FRAGMENT_SIZE_MAX = 1024 * 1024;
  const size_t FRAGMENT_SIZE_MIM = 1024;
  const size_t FRAGMENT_SIZE_STEP = 100 * 1024;

  /*
	const size_t CELL_SIZE_MAX = FRAGMENT_SIZE_MAX;
	const size_t CELL_SIZE_MIM = FRAGMENT_SIZE_MIM;
	const size_t CELL_SIZE_STEP = FRAGMENT_SIZE_STEP;
  */

  const size_t CHUNK_SIZE_MAX = 1024 * 1024;
  const size_t CHUNK_SIZE_MIM = 100;
  const size_t CHUNK_SIZE_STEP = 100;

  std::vector<TestConfig> configurations;

  for (size_t inflight_events = EVENTS_INFLIGHT_MIM;
	   inflight_events < EVENTS_INFLIGHT_MAX;
	   inflight_events += EVENTS_INFLIGHT_STEP) {
	for (size_t fragments_to_save = FRAGMENTS_TO_SAVE_MIM;
		 fragments_to_save < FRAGMENTS_TO_SAVE_MAX;
		 fragments_to_save += FRAGMENTS_TO_SAVE_STEP) {
	  for (size_t fragment_size = FRAGMENT_SIZE_MIM;
		   fragment_size < FRAGMENT_SIZE_MAX;
		   fragment_size += FRAGMENT_SIZE_STEP) {

		for (size_t chunk_size = CHUNK_SIZE_MIM; chunk_size < CHUNK_SIZE_MAX;
			 chunk_size += CHUNK_SIZE_STEP) {
		    configurations.push_back(TestConfig::generate_array_config(
		  	  true, fragment_size, inflight_events, fragments_to_save,
		  	  chunk_size, fragment_size));
		    configurations.push_back(TestConfig::generate_array_config(
		  	  false, fragment_size, inflight_events, fragments_to_save,
		  	  chunk_size, fragment_size));
		}
		configurations.push_back(TestConfig::generate_key_value_config(
			true, fragment_size, inflight_events, fragments_to_save));
		configurations.push_back(TestConfig::generate_key_value_config(
			false, fragment_size, inflight_events, fragments_to_save));
	  }
	}
  }
//   std::cout << RED << "56bytes * " << counter << RESET << std::endl;
  return configurations;
}

int main(int argc, char** argv) {

  std::string pool_uuid_string = "683bce8d-e49b-4961-8a6a-ad899ca9de4a";
  if (argc == 2) {
	pool_uuid_string = argv[1];
  }
  std::vector<TimingInfo> results;
  DAOS_CHECK(daos_init());
  {
	Pool pool(pool_uuid_string);
	// sweep_settings(pool);
	printf("Generating configurations ...");
	auto configurations = generate_configurations();
	printf("done\n");
	Harness harness(configurations, pool);
	results = harness.measure();
  }
  DAOS_CHECK(daos_fini());
  printf(RED "--- Collected %ld data points\n" RESET, results.size());

}