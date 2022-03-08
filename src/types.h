#include <ostream>
#include <uuid/uuid.h>

#ifndef MK_TYPES_H
#define MK_TYPES_H

using UUID = unsigned char *; // uuid_t;

std::string unparse(UUID uuid)
{
	char buffer[36 + 1];
	uuid_unparse(uuid, buffer);
	return buffer;
}


#endif // !MK_TYPES_H