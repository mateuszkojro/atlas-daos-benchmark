#include "DAOSObject.h"
#include "daos_errno.h"
#include "daos_obj.h"
#include <cstddef>
#include <iostream>
#include <ostream>

DAOSObject::DAOSObject(daos_handle_t object_handle, daos_obj_id_t object_id)
	: object_handle_(object_handle), object_id_(object_id) {}

DAOSObject::~DAOSObject() {
  int ret = daos_obj_close(object_handle_, NULL);
  if (ret != DER_SUCCESS) {
	std::cerr << "Object handle could not be closed: " << d_errstr(ret)
			  << std::endl;
  }
}