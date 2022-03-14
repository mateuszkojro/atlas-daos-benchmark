#ifndef MK_EVENT_QUEUE_H
#define MK_EVENT_QUEUE_H

#include "Errors.h"
#include "Pool.h"
#include "daos.h"
#include "daos_event.h"
#include "daos_types.h"
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <stdexcept>

#include <iostream>
#include <vector>

class EventQueue {
public:
  EventQueue(size_t max_inflight);
  EventQueue(EventQueue &&) = default;
  EventQueue(const EventQueue &) = default;
  EventQueue &operator=(EventQueue &&) = default;
  EventQueue &operator=(const EventQueue &) = default;
  ~EventQueue();

  daos_event_t *get_event();
  void wait();

private:
  daos_event_t *pool();
  daos_event_t *add_event();
  size_t inflight_ = 0;
  const size_t max_inflight_;

  daos_handle_t event_queue_handle_;
  std::vector<daos_event_t> events_;
};

EventQueue::EventQueue(size_t max_inflight)
    : max_inflight_(max_inflight), events_(max_inflight) {
  DAOS_CHECK(daos_eq_create(&event_queue_handle_));
}

// FIXME: Throwing in the destructor
EventQueue::~EventQueue() {
  wait();
  int flags = 0; // DAOS_EQ_DESTROY_FORCE
  while (daos_eq_destroy(event_queue_handle_, flags) == EBUSY) {
    // TODO: How to wait for it
    std::cout << "EBUSY" << std::endl;
    continue;
  }
}

void EventQueue::wait() {
  auto *event = pool();
  while (event != nullptr) {
    event = pool();
  }
}

daos_event_t *EventQueue::pool() {
  const int events_to_pool = 1;
  daos_event_t *event[events_to_pool];

  int ret =
      daos_eq_poll(event_queue_handle_, 1, DAOS_EQ_WAIT, events_to_pool, event);

  if (ret == 0) {
    return nullptr;
  }

  if (ret < 0) {
    DAOS_CHECK(ret);
    throw std::runtime_error("Could not pool the events unknown error");
  }

  DAOS_CHECK(event[0]->ev_error);
  event[0]->ev_error = 0; // Reuse the same event

  return *event;
}

daos_event_t *EventQueue::add_event() {
  daos_event_t *event = &events_[inflight_];
  DAOS_CHECK(daos_event_init(event, event_queue_handle_, NULL));
  inflight_++;
  return event;
}

daos_event_t *EventQueue::get_event() {
  if (inflight_ < max_inflight_) {
    return add_event();
  }
  return pool();
}

#endif // !MK_EVENT_QUEUE_H