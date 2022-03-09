#ifndef MK_EVENT_QUEUE_H
#define MK_EVENT_QUEUE_H

#include "Errors.h"
#include "daos.h"
#include "daos_event.h"
#include "daos_types.h"
#include <cstddef>
#include <stdexcept>

class EventQueue {
public:
  EventQueue(size_t max_inflight);
  EventQueue(EventQueue &&) = default;
  EventQueue(const EventQueue &) = default;
  EventQueue &operator=(EventQueue &&) = default;
  EventQueue &operator=(const EventQueue &) = default;
  ~EventQueue();

  daos_event_t get_event();

private:
  daos_event_t pool();
  daos_event_t add_event();
  size_t inflight_ = 0;
  const size_t max_inflight_;

  daos_handle_t event_queue_handle_;
};

EventQueue::EventQueue(size_t max_inflight) : max_inflight_(max_inflight) {
  DAOS_CHECK(daos_eq_create(&event_queue_handle_));
}

EventQueue::~EventQueue() {
  int flags = 0;
  daos_eq_destroy(event_queue_handle_, flags);
}

daos_event_t EventQueue::pool() {
  daos_event_t *event;
  unsigned int events_to_pool;
  int ret = daos_eq_poll(event_queue_handle_, 1, DAOS_EQ_WAIT, events_to_pool,
                         &event);
  if (ret == 0) {
    throw std::runtime_error("Event queue empty");
  }
  if (ret < 0) {
    throw std::runtime_error("Error pooling event");
  }
  DAOS_CHECK(event->ev_error);
  event->ev_error = 0; // Reuse the same event
  return *event;
}

daos_event_t EventQueue::add_event() {
  daos_event_t event;
  DAOS_CHECK(daos_event_init(&event, event_queue_handle_, NULL));
  inflight_++;
  return event;
}

daos_event_t EventQueue::get_event() {
  if (inflight_ < max_inflight_) {
    return add_event();
  }
  return pool();
}

#endif // !MK_EVENT_QUEUE_H