#ifndef MK_MOCK_POOL_H
#define MK_MOCK_POOL_H

#include "Pool.h"
#include "interfaces.h"
#include "mocking.h"
#include "MockContainer.h"
#include <memory>

class MockPool : public IPool, public MockDAOSObj {
 public:
  MockPool(std::weak_ptr<MockDAOS> mock_daos) : MockDAOSObj(mock_daos) {
    this->mock_daos()->daos_pool_connect_cpp();
  }

  virtual ContainerPtr add_container(const std::string& name = "") {
	mock_daos()->daos_cont_create_cpp();
	return std::make_unique<MockContainer>(mock_daos());
  }
  virtual ContainerPtr get_container(const UUID& uuid) { MK_UNIMPLEMENTED; };
  virtual void remove_container(ContainerPtr& ptr) { mock_daos()->daos_cont_destroy_cpp(); };
  virtual void connect(const unsigned char* uuid_or_label) {
	mock_daos()->daos_pool_connect_cpp();
  }
  virtual void connect(const std::string& label) {
	mock_daos()->daos_pool_connect_cpp();
  }
  virtual std::unique_ptr<daos_handle_t> get_pool_handle() {
	MK_UNIMPLEMENTED;
  };
  virtual void clean_up() { MK_UNIMPLEMENTED; };

  virtual ~MockPool() { mock_daos()->daos_cont_close(); }
};

#endif// !MK_MOCK_POOL_H
