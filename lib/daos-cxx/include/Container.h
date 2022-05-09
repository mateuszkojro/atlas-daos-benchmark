#ifndef MK_CONTAINER_H
#define MK_CONTAINER_H

#include "Errors.h"
#include "KeyValue.h"
#include <cstddef>
#include <iostream>
#include <memory>
#include <system_error>
#include <vector>

#include "Array.h"
#include "UUID.h"
#include "daos_array.h"
#include "daos_cont.h"
#include "daos_obj_class.h"
#include "daos_prop.h"
#include "daos_types.h"
#include "interfaces.h"
#include <daos.h>
#include <daos_errno.h>
#include <daos_obj.h>

class Container : public IContainer {
 public:
  Container(UUID uuid, daos_handle_t pool_handle);
  Container(const std::string& container_label, daos_handle_t pool_handle);
  Container(daos_handle_t container_handle);

  Container(Container&&) = default;
  Container(const Container&) = delete;
  Container& operator=(Container&&) = default;
  Container& operator=(const Container&) = delete;
  ~Container();

  UUID get_uuid();

  KeyValuePtr create_kv_object();

  // TODO: Cell size and chunk size can be diferent
  ArrayPtr create_array();

 private:
  daos_handle_t container_handle_;
  // std::vector<DAOSObject> objects_;
};
#endif// !MK_CONTAINER_H
