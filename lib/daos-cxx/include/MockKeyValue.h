#include "daos_types.h"
#include "interfaces.h"
#include "mocking.h"
#include <memory>

class MockKeyValue : public IKeyValue, public MockDAOSObj {

  MockKeyValue(const std::weak_ptr<MockDAOS>& mock_daos)
	  : MockDAOSObj(mock_daos) {}

  virtual void write_raw(const char* key, const char* value, size_t value_size,
						 daos_event_t* event = NULL) {

	mock_daos()->daos_kv_put(mock_daos()->get_mock_handle(), DAOS_TX_NONE, 0,
							 key, value_size, value, event);
  }

  virtual void read_raw(const char* key) {
	const size_t buffer_size = 128;
	char value[buffer_size];
	size_t read_bytes = buffer_size;
	mock_daos()->daos_kv_get(mock_daos()->get_mock_handle(), DAOS_TX_NONE, 0,
							 key, &read_bytes, &value, NULL);
  }
};