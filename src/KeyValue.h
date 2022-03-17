#ifndef M_KEY_VALUE_H
#define M_KEY_VALUE_H

#include "DAOSObject.h"
#include "Errors.h"
#include "daos_kv.h"
#include "daos_types.h"
#include <cstddef>
#include <cstdlib>
#include <stdexcept>

#include <iostream>
#include <string>

class KeyValue : public DAOSObject {
public:
  KeyValue(daos_handle_t object_handle, daos_obj_id_t object_id);
  KeyValue(KeyValue &&) = default;
  KeyValue(const KeyValue &) = default;
  KeyValue &operator=(KeyValue &&) = default;
  KeyValue &operator=(const KeyValue &) = default;
  ~KeyValue() = default;

  void put_raw(daos_event_t *event, const char *key, const char *value,
               size_t value_size);
  void get_raw(daos_event_t *event, const char *key);

  void change_value_raw();

private:
};

KeyValue::KeyValue(daos_handle_t object_handle, daos_obj_id_t object_id)
    : DAOSObject(object_handle, object_id) {}

std::string random_string() {
  static std::size_t index = 0;
  index++;
  return std::to_string(index);
}

void KeyValue::put_raw(daos_event_t *event, const char *key, const char *value,
                       size_t value_size) {
  DAOS_CHECK(daos_kv_put(object_handle_, DAOS_TX_NONE, 0, key, value_size,
                         value, event));
}

void KeyValue::change_value_raw() {
  throw std::runtime_error("Not implemented");
}

void KeyValue::get_raw(daos_event_t *event, const char *key) {
  size_t buffer_size = 128;
  char value[128];
  DAOS_CHECK(daos_kv_get(object_handle_, DAOS_TX_NONE, 0, key, &buffer_size,
                         &value, event));

  std::cout << "Value: " << value << std::endl;
}

#endif //  M_KEY_VALUE_H