#ifndef MK_MOCK_CONTAINER_H
#define MK_MOCK_CONTAINER_H

#include "MockKeyValue.h"
#include "interfaces.h"
#include "mocking.h"
#include <memory>

class MockContainer : public MockDAOSObj, public IContainer {
 public:
  MockContainer(std::weak_ptr<MockDAOS> mock_daos) : MockDAOSObj(mock_daos) {}

  virtual UUID get_uuid() { MK_UNIMPLEMENTED; }
  virtual KeyValuePtr create_kv_object() {
	mock_daos()->daos_kv_open();
	return std::make_unique<MockKeyValue>(mock_daos());
  }
  // TODO: Cell size and chunk size can be diferent
  virtual ArrayPtr create_array() { MK_UNIMPLEMENTED; }
};

#endif// !MK_MOCK_CONTAINER_H