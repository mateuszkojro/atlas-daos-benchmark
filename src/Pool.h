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

using ContainerPtr = std::unique_ptr<Container>;

class Pool
{
      public:
	Pool(UUID pool_uuid);
	Pool(Pool &&)      = delete;
	Pool(const Pool &) = delete;
	Pool &operator=(Pool &&) = delete;
	Pool &operator=(const Pool &) = delete;
	~Pool();

	ContainerPtr add_container(const std::string &name = "");
	ContainerPtr get_container(const UUID& uuid);
	void         remove_container(ContainerPtr &ptr);
	void         connect();
	void         clean_up();

      private:
	UUID          pool_uuid_;
	daos_handle_t pool_handle_;
};

Pool::Pool(UUID pool_uuid) : pool_uuid_(pool_uuid) { connect(); }

// TODO: Closing all the connections
Pool::~Pool() { clean_up(); }

void Pool::connect()
{
	unsigned flags = DAOS_PC_RW;
	DAOS_CHECK(daos_pool_connect(pool_uuid_.raw(), nullptr, flags, &pool_handle_,
				     nullptr, nullptr));
}

void Pool::clean_up() { DAOS_CHECK(daos_pool_disconnect(pool_handle_, nullptr)); }

ContainerPtr Pool::add_container(const std::string &name)
{
	// daos_prop_t cont_prop;
	uuid_t      container_uuid;

	if (name.empty()) {
		DAOS_CHECK(daos_cont_create_cpp(pool_handle_, &container_uuid, nullptr, nullptr));
	} else {
		DAOS_CHECK(daos_cont_create_with_label(pool_handle_, name.c_str(), NULL,
						       &container_uuid, NULL));
	}

	return std::make_unique<Container>(container_uuid, pool_handle_);
}

// TODO: not tested!
ContainerPtr Pool::get_container(const UUID& uuid){
	daos_handle_t container_handle;
	int flags = 0;
	DAOS_CHECK(daos_cont_open(pool_handle_, uuid.raw(), flags, &container_handle,NULL, NULL)); 
	return std::make_unique<Container>(container_handle);
}

void Pool::remove_container(ContainerPtr &ptr)
{
	bool force_destroy = true;
	DAOS_CHECK(
	    daos_cont_destroy(pool_handle_, ptr->get_uuid().raw(), force_destroy, NULL));
}

#endif // !MK_POOL_H
