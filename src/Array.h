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

#define CELL_SIZE 1024
#define CHUNK_SIZE 1024 * 100

class Array : public DAOSObject {
public:
  Array(daos_handle_t array_handle, daos_obj_id_t object_id);
  Array(const Array &other) = delete;
  Array(Array &&) = default;

  const Array &operator=(const Array &other) = delete;

  void write_raw(size_t idx, char *buffer, daos_event_t *event = NULL);
  void read_raw(size_t idx);

private:
  // const size_t cell_size_;
  // const size_t chunk_size_;
};

Array::Array(daos_handle_t array_handle, daos_obj_id_t object_id)
    : DAOSObject(array_handle, object_id) {}

// TODO: There is a lot of optimisation that can be done with array acceses
void Array::write_raw(size_t idx, char *buffer, daos_event_t *event) {
  // TODO: check bounds here
  daos_array_iod_t ranges_descriptor; // List of idx ranges
  ranges_descriptor.arr_nr = 1;       // How many ranges to write

  daos_range_t range; // Index range wraper
  range.rg_idx = idx; // Idx in a array
  range.rg_len = 1;   // Range len

  daos_range_t ranges[1] = {range};
  ranges_descriptor.arr_rgs = ranges;

  d_iov_t iov;                 // Memory buffer wraper
  iov.iov_buf = buffer;        // Buffer ptr
  iov.iov_len = CELL_SIZE;     // Data len
  iov.iov_buf_len = CELL_SIZE; // Buffer size

  d_iov_t iov_array[1] = {iov};

  d_sg_list_t scatter_gather_list;         // List of memory buffers?
  scatter_gather_list.sg_nr = 1;           // FIXME: No idea
  scatter_gather_list.sg_nr_out = 1;       // FIXME: No idea
  scatter_gather_list.sg_iovs = iov_array; // Ptr to array of buffers

  DAOS_CHECK(daos_array_write(object_handle_, DAOS_TX_NONE, &ranges_descriptor,
                              &scatter_gather_list, event));
}

// TODO: There is a lot of optimisation that can be done with array acceses
void Array::read_raw(size_t idx) {
  // assert(false && "This is not implemented");

  std::array<char, CELL_SIZE> buffer;
  daos_array_iod_t ranges_descriptor; // List of idx ranges
  ranges_descriptor.arr_nr = 1;       // How many ranges to write

  daos_range_t range; // Index range wraper
  range.rg_idx = idx; // Idx in a array
  range.rg_len = 1;   // Range len

  daos_range_t ranges[1] = {range};
  ranges_descriptor.arr_rgs = ranges;

  d_iov_t iov;                 // Memory buffer wraper
  iov.iov_buf = buffer.data(); // Ptr to a buffer
  iov.iov_len = CELL_SIZE;     // Data len
  iov.iov_buf_len = CELL_SIZE; // Buffer size

  d_iov_t iov_array[1] = {iov};

  d_sg_list_t scatter_gather_list;         // List of memory buffers?
  scatter_gather_list.sg_nr = 1;           // FIXME: No idea
  scatter_gather_list.sg_nr_out = 1;       // FIXME: No idea
  scatter_gather_list.sg_iovs = iov_array; // Ptr to array of buffers

  DAOS_CHECK(daos_array_read(object_handle_, DAOS_TX_NONE, &ranges_descriptor,
                             &scatter_gather_list, NULL));
  std::cout << ((char *)scatter_gather_list.sg_iovs[0].iov_buf)[0] << std::endl;
}

#endif