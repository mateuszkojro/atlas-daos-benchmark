#ifndef MK_ARRAY_H
#define MK_ARRAY_H

#include "daos.h"
#include <array>
#include <cassert>
#include <iostream>

#include "DAOSObject.h"
#include "daos_array.h"
#include "daos_obj_class.h"
#include "daos_types.h"
#include "gurt/types.h"
#include <cstddef>
#include <cstdlib>
#include <daos.h>
#include <vector>

#include "Errors.h"
#include "interfaces.h"

#define CELL_SIZE 1024
#define CHUNK_SIZE 1024 * 100

class Array : public DAOSObject, public IArray {
 public:
  Array(daos_handle_t array_handle, daos_obj_id_t object_id);
  Array(const Array& other) = delete;
  Array(Array&&) = default;

  const Array& operator=(const Array& other) = delete;

  void write_raw(size_t idx, char* buffer, daos_event_t* event = NULL);
  void read_raw(size_t idx);
};
#endif