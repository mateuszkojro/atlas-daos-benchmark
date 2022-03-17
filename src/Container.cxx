#include "Container.h"

Container::Container(UUID uuid, daos_handle_t pool_handle) {
  DAOS_CHECK(daos_cont_open(pool_handle, uuid.raw(), DAOS_COO_RW,
							&container_handle_, NULL, NULL));
}

Container::Container(daos_handle_t container_handle)
	: container_handle_(container_handle) {}

Container::~Container() {
  // FIXME: I am not rly sure what to do with that
  int error_code = daos_cont_close(container_handle_, NULL);
  if (error_code != DER_SUCCESS) {
	std::cerr << "Container could not be closed properly: "
			  << d_errdesc(error_code) << std::endl;
  }
}

KeyValue Container::create_kv_object() {

  /**
   * TODO:
   * ID of an object, 128 bits
   * The high 32-bit of daos_obj_id_t::hi are reserved for DAOS, the rest is
   * provided by the user and assumed to be unique inside a container.
   */
  daos_obj_id_t object_id;

  // TODO: Look at the cid parameter in the documentation it could speed things
  // up
  DAOS_CHECK(daos_obj_generate_oid_cpp(container_handle_, &object_id,
									   DAOS_OT_KV_LEXICAL, OC_UNKNOWN, 0, 0));

  daos_handle_t object_handle;
  DAOS_CHECK(daos_kv_open(container_handle_, object_id, DAOS_OO_RW,
						  &object_handle, NULL));

  // FIXME: when destructor of kv is called object is destructed and everything
  // goes away
  KeyValue kv(object_handle, object_id);
  objects_.push_back(kv);
  return kv;
}

Array Container::create_array() {
  /**
   *TODO:
   * ID of an object, 128 bits
   * The high 32-bit of daos_obj_id_t::hi are reserved for DAOS, the rest is
   * provided by the user and assumed to be unique inside a container.
   */
  daos_obj_id_t object_id;
  bool daos_mantains_element_and_chunk_size = true;
  DAOS_CHECK(daos_array_generate_oid(container_handle_, &object_id,
									 daos_mantains_element_and_chunk_size,
									 OC_UNKNOWN, 0, 0));
  daos_handle_t array_handle;
  daos_array_create(container_handle_, object_id, DAOS_TX_NONE, CELL_SIZE,
					CHUNK_SIZE, &array_handle, NULL);

  return Array(array_handle, object_id);
}

UUID Container::get_uuid() {
  daos_cont_info_t container_info;
  DAOS_CHECK(daos_cont_query(container_handle_, &container_info, NULL, NULL));
  return container_info.ci_uuid;
}
