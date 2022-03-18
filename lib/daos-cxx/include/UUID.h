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
  UUID(const UUID&);
  const UUID& operator=(const UUID&) = delete;

  const char* raw() const;
  std::string to_string() const;

 private:
  uuid_t uuid_;
};
#endif// MK_!UUID_H