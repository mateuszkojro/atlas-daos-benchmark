#ifndef MK_CONTAINER_H
#define MK_CONTAINER_H

#include "DAOSObject.h"
#include "Errors.h"
#include <cstddef>
#include <vector>

#include "daos_cont.h"
#include "daos_types.h"
#include "types.h"
#include <daos.h>
#include <daos_errno.h>
#include <daos_obj.h>

class Container {
public:
  Container(UUID uuid, daos_handle_t pool_handle);
  Container(Container &&) = default;
  Container(const Container &) = default;
  Container &operator=(Container &&) = default;
  Container &operator=(const Container &) = default;
  ~Container();

  UUID get_uuid() { return container_uuid_; }

  DAOSObject create_kv_object();

private:
  UUID container_uuid_;
  daos_handle_t container_handle_;
  std::vector<DAOSObject> objects_;
};

Container::Container(UUID uuid, daos_handle_t pool_handle)
    : container_uuid_(uuid) {
  DAOS_CHECK(daos_cont_open(pool_handle, (const char *)uuid, DAOS_COO_RW,
                            &container_handle_, NULL, NULL));
}

Container::~Container() {}

DAOSObject Container::create_kv_object() {

  daos_obj_id_t object_id;

  // TODO: Look at the cid parameter in the documentation it coul speed things
  // up
  daos_obj_generate_oid_cpp(container_handle_, &object_id, DAOS_OT_DKEY_UINT64,
                            0, 0, 0);

  daos_handle_t object_handle;
  daos_obj_open(container_handle_, object_id, DAOS_OO_RW, &object_handle, NULL);

  objects_.push_back(DAOSObject(object_handle, object_id));
  return objects_.back();
}

#endif // !MK_CONTAINER_H