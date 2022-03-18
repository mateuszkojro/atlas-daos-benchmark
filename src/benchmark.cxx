#include "Array.h"
#include "Container.h"
#include "EventQueue.h"
#include "Pool.h"
#include "daos_event.h"
#include "daos_types.h"
#include <cctype>
#include <chrono>
#include <cstddef>
#include <exception>
#include <iostream>
#include <ratio>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#define KB(x) 1024 * x
#define MB(x) 1024 * 1024 * x

using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::microseconds;

const int number_ok_events_to_save = 1000;
const int event_size = CELL_SIZE;// MB(5);

void test_kv(ContainerPtr& container, EventQueue& event_queue) {
  auto object = container->create_kv_object();

  auto start = high_resolution_clock::now();
  for (int i = 0; i < number_ok_events_to_save; i++) {
	object->write_raw(std::to_string(i).c_str(), "bb", event_size,
					 event_queue.get_event());
  }

  event_queue.wait();
  auto end = high_resolution_clock::now();
  std::cout << "Using event queue:\t"
			<< duration_cast<microseconds>(end - start).count() << "us\n";

  start = high_resolution_clock::now();
  for (int i = 0; i < number_ok_events_to_save; i++) {
	object->write_raw(std::to_string(i + number_ok_events_to_save).c_str(), "bb",
					 event_size, NULL);
  }
  end = high_resolution_clock::now();
  std::cout << "Without event queue:\t"
			<< duration_cast<microseconds>(end - start).count() << "us\n";
}

void test_array(ContainerPtr& container, EventQueue& event_queue) {
  auto object = container->create_array();

  char* array = new char[CELL_SIZE];
  array[0] = 'A';

  auto start = high_resolution_clock::now();
  for (int i = 0; i < number_ok_events_to_save; i++) {
	object->write_raw(i, array);
  }
  auto end = high_resolution_clock::now();
  std::cout << "Array w/o eq:\t"
			<< duration_cast<microseconds>(end - start).count() << "us"
			<< std::endl;

  start = high_resolution_clock::now();
  for (int i = 0; i < number_ok_events_to_save; i++) {
	object->write_raw(i, array, event_queue.get_event());
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

int main(int argc, char** argv) {

  std::string pool_uuid_string = "7b5abd84-4f94-497e-aca3-c5eeca5503f9";
  if (argc == 2) {
	pool_uuid_string = argv[1];
  }

  DAOS_CHECK(daos_init());
  {
	Pool pool(pool_uuid_string);
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
  DAOS_CHECK(daos_fini());
}