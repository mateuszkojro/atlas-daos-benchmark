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

int main(int argc, char **argv) {

  std::string pool_uuid_string = "7b5abd84-4f94-497e-aca3-c5eeca5503f9";
  if (argc == 2) {
    pool_uuid_string = argv[1];
  }

  DAOS_CHECK(daos_init());
  {
    Pool pool(pool_uuid_string);
    auto container = pool.add_container("kv_put_test");

    const int inflight = 1000;
    EventQueue event_queue(inflight);

    auto object = container->create_kv_object();
    int number_ok_events_to_save = 1;
    int event_size = 10; // MB(5);

    try {
      auto start = std::chrono::high_resolution_clock::now();
      for (int i = 0; i < number_ok_events_to_save; i++) {
        object.put_raw(event_queue.get_event(), std::to_string(i).c_str(), "bb",
                       event_size);
      }
      event_queue.wait();
      auto end = std::chrono::high_resolution_clock::now();
      std::cout << "Using event queue:\t"
                << duration_cast<std::chrono::microseconds>(end - start).count()
                << "us\n";

      start = std::chrono::high_resolution_clock::now();
      for (int i = 0; i < number_ok_events_to_save; i++) {
        object.put_raw(NULL,
                       std::to_string(i + number_ok_events_to_save).c_str(),
                       "bb", event_size);
      }
      end = std::chrono::high_resolution_clock::now();
      std::cout << "Without event queue:\t"
                << std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                         start)
                       .count()
                << "us\n";
    } catch (std::exception &e) {
      std::cout << "Error occurred: " << e.what() << std::endl;
    }

    pool.remove_container(container);
  }
  DAOS_CHECK(daos_fini());

  std::cout << "All operations were ok" << std::endl;
}