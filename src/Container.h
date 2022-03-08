#ifndef MK_CONTAINER_H
#define MK_CONTAINER_H

#include <daos.h>
#include <daos_errno.h>
#include "types.h"

class Container
{
      public:
	Container(UUID uuid);
	Container(Container &&)      = default;
	Container(const Container &) = default;
	Container &operator=(Container &&) = default;
	Container &operator=(const Container &) = default;
	~Container();

	UUID get_uuid() { return container_uuid_; }

      private:
	UUID container_uuid_;
};

Container::Container(UUID uuid) : container_uuid_(uuid) {}

Container::~Container() {}

#endif // !MK_CONTAINER_H