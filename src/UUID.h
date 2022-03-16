#ifndef MK_UUID_H
#define MK_UUID_H

#include <memory>
#include <string>
#include <uuid/uuid.h>
class UUID {
public:
  UUID(uuid_t uuid);
  UUID(std::string uuid);
  UUID(UUID&&) = default;
  UUID(const UUID &);
  const UUID &operator=(const UUID &) = delete;

  const char *raw() const;
  std::string to_string() const;

private:
  uuid_t uuid_;
};

UUID::UUID(uuid_t uuid) { uuid_copy(uuid_, uuid); }
UUID::UUID(std::string uuid) { uuid_parse(uuid.c_str(), uuid_); }

UUID::UUID(const UUID &other) { uuid_copy(uuid_, other.uuid_); }

const char *UUID::raw() const { return reinterpret_cast<const char *>(uuid_); }

std::string UUID::to_string() const {
  char buffer[36 + 1];
  uuid_unparse((unsigned char *)uuid_, buffer);
  return buffer;
}

#endif // MK_!UUID_H