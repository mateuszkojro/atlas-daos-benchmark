#include "Container.h"
#include "KeyValue.h"
#include <memory>
#include <string>

Container::Container(UUID uuid, daos_handle_t pool_handle) {
  MK_UNIMPLEMENTED;
  uuid_t container_uuid;
  DAOS_CHECK(daos_cont_open(pool_handle, "benchmark_container", DAOS_COO_RW,
							&container_handle_, NULL, NULL));
}

Container::Container(const std::string& container_label,
					 daos_handle_t pool_handle) {
  DAOS_CHECK(daos_cont_open(pool_handle, container_label.c_str(), DAOS_COO_RW,
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

KeyValuePtr Container::create_kv_object() {

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

  return std::make_unique<KeyValue>(object_handle, object_id);
}

ArrayPtr Container::create_array() {
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

  return std::make_unique<Array>(array_handle, object_id);
}

UUID Container::get_uuid() {
  daos_cont_info_t container_info;
  DAOS_CHECK(daos_cont_query(container_handle_, &container_info, NULL, NULL));
  return container_info.ci_uuid;
}
