#include "DAOSObject.h"

DAOSObject::DAOSObject(daos_handle_t object_handle, daos_obj_id_t object_id)
	: object_handle_(object_handle), object_id_(object_id) {}