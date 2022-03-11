#ifndef MK_UUID_H
#define MK_UUID_H

#include <memory>
#include <uuid/uuid.h>
class UUID {
public:
  UUID(uuid_t uuid);
  UUID(std::string uuid);
  UUID(const UUID &);
  const UUID &operator=(const UUID &) = delete;

  const char *raw();

private:
  uuid_t uuid_;
};

UUID::UUID(uuid_t uuid) { uuid_copy(uuid_, uuid); }
UUID::UUID(std::string uuid) { uuid_parse(uuid.c_str(), uuid_); }

UUID::UUID(const UUID &other) { uuid_copy(uuid_, other.uuid_); }

const char *UUID::raw() { return reinterpret_cast<const char *>(uuid_); }

static std::string unparse(UUID uuid) {
  char buffer[36 + 1];
  uuid_unparse((unsigned char *)uuid.raw(), buffer);
  return buffer;
}

#endif // MK_!UUID_H