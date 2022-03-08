#ifndef MK_POOL_H
#define MK_POOL_H

#include <daos.h>
#include <daos_errno.h>

#include <memory>

#include "Container.h"
#include "Errors.h"
#include "types.h"

using ContainerPtr = std::unique_ptr<Container>;

class Pool
{
      public:
	Pool(UUID pool_uuid);
	Pool(Pool &&)      = default;
	Pool(const Pool &) = default;
	Pool &operator=(Pool &&) = default;
	Pool &operator=(const Pool &) = default;
	~Pool();

	ContainerPtr add_container(const std::string &name = "");
	ContainerPtr get_container(UUID uuid);
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
	DAOS_CHECK(daos_pool_connect((const char *)pool_uuid_, nullptr, flags, &pool_handle_,
				     nullptr, nullptr));
}

void Pool::clean_up() { DAOS_CHECK(daos_pool_disconnect(pool_handle_, nullptr)); }

ContainerPtr Pool::add_container(const std::string &name)
{
	uuid_t      container_uuid;
	daos_prop_t cont_prop;

	if (name.empty()) {
		DAOS_CHECK(daos_cont_create_cpp(pool_handle_, &container_uuid, nullptr, nullptr));
	} else {
		DAOS_CHECK(daos_cont_create_with_label(pool_handle_, name.c_str(), NULL,
						       &container_uuid, NULL));
	}

	return std::make_unique<Container>(container_uuid);
}

// FIXME: Removin is not working and I dont rly know why
void Pool::remove_container(ContainerPtr &ptr)
{
	bool force_destroy = true;
	DAOS_CHECK(
	    daos_cont_destroy(pool_handle_, (const char *)ptr->get_uuid(), force_destroy, NULL));
}

#endif // !MK_POOL_H