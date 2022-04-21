#ifndef MK_INTERFACES
#define MK_INTERFACES

#include "UUID.h"
#include "daos_types.h"
#include <cassert>
#include <cstddef>
#include <memory>

#define MK_UNIMPLEMENTED                                                       \
  do {                                                                         \
	assert(false && "Unimplemented");                                          \
	throw std::runtime_error("Unimplemented");                                 \
  } while (false)

class IKeyValue {
 public:
  virtual void write_raw(const char* key, const char* value, size_t value_size,
						 daos_event_t* event = NULL) = 0;

  virtual void read_raw(const char* key) = 0;
  virtual ~IKeyValue() = default;
};

class IArray {
 public:
  virtual void write_raw(size_t idx, char* buffer,
						 daos_event_t* event = NULL) = 0;
  virtual void read_raw(size_t idx) = 0;
  virtual ~IArray() = default;
};

using KeyValuePtr = std::unique_ptr<IKeyValue>;
using ArrayPtr = std::unique_ptr<IArray>;

class IContainer {
 public:
  virtual UUID get_uuid() = 0;
  virtual KeyValuePtr create_kv_object() = 0;

  // TODO: Cell size and chunk size can be diferent
  virtual ArrayPtr create_array() = 0;
  virtual ~IContainer() = default;
};

using ContainerPtr = std::unique_ptr<IContainer>;

class IPool {
 public:
  virtual ContainerPtr add_container(const std::string& name = "") = 0;
  virtual ContainerPtr get_container(const UUID& uuid) = 0;
  virtual void remove_container(ContainerPtr& ptr) = 0;
  virtual void connect(const unsigned char* uuid_or_label) = 0;
  virtual void connect(const std::string& label) = 0;
  virtual std::unique_ptr<daos_handle_t> get_pool_handle() = 0;
  virtual void clean_up() = 0;
  virtual ~IPool() = default;
};

#endif// !MK_INTERFACES
