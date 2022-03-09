#ifndef MK_DAOS_OBJECT_H
#define MK_DAOS_OBJECT_H

#include "daos_types.h"
class DAOSObject {
public:
  DAOSObject(daos_handle_t object_handle, daos_obj_id_t object_id);
  DAOSObject(DAOSObject &&) = default;
  DAOSObject(const DAOSObject &) = default;
  DAOSObject &operator=(DAOSObject &&) = default;
  DAOSObject &operator=(const DAOSObject &) = default;
  ~DAOSObject();

private:
  daos_handle_t object_handle_;
  daos_obj_id_t object_id_;
};

DAOSObject::DAOSObject(daos_handle_t object_handle, daos_obj_id_t object_id)
    : object_handle_(object_handle), object_id_(object_id) {}

DAOSObject::~DAOSObject() {}

#endif // MK_!DAOS_OBJECT_H