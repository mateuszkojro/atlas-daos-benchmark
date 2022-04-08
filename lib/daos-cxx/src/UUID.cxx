#include "UUID.h"

UUID::UUID(uuid_t uuid) { uuid_copy(uuid_, uuid); }

UUID::UUID(std::string uuid) { uuid_parse(uuid.c_str(), uuid_); }

UUID::UUID(const UUID& other) { uuid_copy(uuid_, other.uuid_); }

const unsigned char * UUID::raw() const { 
  return static_cast<const unsigned char *>(uuid_);
}

std::string UUID::to_string() const {
  char buffer[36 + 1];
  uuid_unparse((unsigned char*)uuid_, buffer);
  return buffer;
}