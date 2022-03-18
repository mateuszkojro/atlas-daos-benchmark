#ifndef MK_DAOS_OBJECT_H
#define MK_DAOS_OBJECT_H

#include "daos_types.h"

class DAOSObject {
 public:
  DAOSObject(daos_handle_t object_handle, daos_obj_id_t object_id);
  DAOSObject(DAOSObject&&) = default;
  DAOSObject(const DAOSObject&) = default;
  DAOSObject& operator=(DAOSObject&&) = default;
  DAOSObject& operator=(const DAOSObject&) = default;
  ~DAOSObject();

 protected:
  // FXIME: This handles should be closed
  daos_handle_t object_handle_;
  daos_obj_id_t object_id_;
};

#endif// MK_!DAOS_OBJECT_H