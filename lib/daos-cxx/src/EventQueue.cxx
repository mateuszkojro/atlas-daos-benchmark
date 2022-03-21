#include "EventQueue.h"

EventQueue::EventQueue(size_t max_inflight)
	: max_inflight_(max_inflight), events_(max_inflight) {
  DAOS_CHECK(daos_eq_create(&event_queue_handle_));
}

// FIXME: Throwing in the destructor
EventQueue::~EventQueue() {
  wait();
  int flags = 0;// DAOS_EQ_DESTROY_FORCE
  while (daos_eq_destroy(event_queue_handle_, flags) == EBUSY) {
	// TODO: How to wait for it
	std::cout << "EBUSY" << std::endl;
	continue;
  }
}

void EventQueue::wait() {
  auto* event = pool();
  while (event != nullptr) { event = pool(); }
}

daos_event_t* EventQueue::pool() {
  const int events_to_pool = 1;
  daos_event_t* event[events_to_pool];

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
  event[0]->ev_error = 0;// Reuse the same event

  return *event;
}

daos_event_t* EventQueue::add_event() {
  daos_event_t* event = &events_[inflight_++];
  DAOS_CHECK(daos_event_init(event, event_queue_handle_, NULL));
  // inflight_++;
  return event;
}

daos_event_t* EventQueue::get_event() {
  if (inflight_ < max_inflight_) {
	return add_event();
  }
  return pool();
}
