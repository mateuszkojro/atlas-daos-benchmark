#ifndef MK_CONTAINER_H
#define MK_CONTAINER_H

#include "Errors.h"
#include "KeyValue.h"
#include <cstddef>
#include <vector>

#include "UUID.h"
#include "daos_cont.h"
#include "daos_obj_class.h"
#include "daos_types.h"
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

  KeyValue create_kv_object();

private:
  UUID container_uuid_;
  daos_handle_t container_handle_;
  std::vector<DAOSObject> objects_;
};

Container::Container(UUID uuid, daos_handle_t pool_handle)
    : container_uuid_(uuid) {
  DAOS_CHECK(daos_cont_open(pool_handle, uuid.raw(), DAOS_COO_RW,
                            &container_handle_, NULL, NULL));
}

Container::~Container() {
  // FIXME: Thar is *very* bad This way destructor can throw
  DAOS_CHECK(daos_cont_close(container_handle_, NULL));
}

KeyValue Container::create_kv_object() {

  daos_obj_id_t object_id;

  // TODO: Look at the cid parameter in the documentation it could speed things
  // up
  DAOS_CHECK(daos_obj_generate_oid_cpp(container_handle_, &object_id,
                                       DAOS_OT_KV_LEXICAL, OC_UNKNOWN, 0, 0));

  daos_handle_t object_handle;
  DAOS_CHECK(daos_kv_open(container_handle_, object_id, DAOS_OO_RW,
                          &object_handle, NULL));

  KeyValue kv(object_handle, object_id);
  objects_.push_back(kv);
  return kv;
}

#endif // !MK_CONTAINER_H