#ifndef MK_POOL_H
#define MK_POOL_H

#include <cstddef>
#include <daos.h>
#include <daos_errno.h>

#include <memory>
#include <string>

#include "Container.h"
#include "Errors.h"
#include "daos_cont.h"
#include "daos_types.h"
#include "types.h"

class Pool : public IPool {
 public:
  Pool(UUID pool_uuid);
  Pool(const std::string pool_label);
  Pool(Pool&&) = delete;
  Pool(const Pool&) = delete;
  Pool& operator=(Pool&&) = delete;
  Pool& operator=(const Pool&) = delete;
  ~Pool();

  ContainerPtr add_container(const std::string& name = "");
  ContainerPtr get_container(const UUID& uuid);
  void remove_container(ContainerPtr& ptr);
  void connect(const unsigned char* uuid_or_label);
  void connect(const std::string& label);
  std::unique_ptr<daos_handle_t> get_pool_handle();
  void clean_up();

 private:
  // UUID pool_uuid_;
  daos_handle_t pool_handle_;
};

#endif// !MK_POOL_H
