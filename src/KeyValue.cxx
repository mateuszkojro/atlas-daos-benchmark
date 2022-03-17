#include "KeyValue.h"

KeyValue::KeyValue(daos_handle_t object_handle, daos_obj_id_t object_id)
	: DAOSObject(object_handle, object_id) {}

std::string random_string() {
  static std::size_t index = 0;
  index++;
  return std::to_string(index);
}

void KeyValue::write_raw(const char* key, const char* value, size_t value_size,
						 daos_event_t* event) {

  DAOS_CHECK(daos_kv_put(object_handle_, DAOS_TX_NONE, 0, key, value_size,
						 value, event));
}

void KeyValue::change_value_raw() {
  throw std::runtime_error("Not implemented");
}

void KeyValue::read_raw(const char* key) {
  size_t buffer_size = 128;
  char value[128];
  DAOS_CHECK(daos_kv_get(object_handle_, DAOS_TX_NONE, 0, key, &buffer_size,
						 &value, NULL));

  std::cout << "Value: " << value << std::endl;
}
