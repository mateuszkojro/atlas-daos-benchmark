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
  KeyValue(KeyValue&&) = default;
  KeyValue(const KeyValue&) = default;
  KeyValue& operator=(KeyValue&&) = default;
  KeyValue& operator=(const KeyValue&) = default;
  ~KeyValue() = default;

  void put_raw(daos_event_t* event, const char* key, const char* value,
			   size_t value_size);
  void get_raw(daos_event_t* event, const char* key);

  void change_value_raw();

 private:
};

#endif//  M_KEY_VALUE_H