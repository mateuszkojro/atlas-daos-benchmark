#include "daos_types.h"
#include "interfaces.h"
#include "mocking.h"
#include <cstddef>
#include <memory>

class MockKeyValue : public IKeyValue, public MockDAOSObj {
 public:
  MockKeyValue(const std::weak_ptr<MockDAOS>& mock_daos)
	  : MockDAOSObj(mock_daos) {}

  virtual void write_raw(const char* key, const char* value, size_t value_size,
						 daos_event_t* event = NULL) {

	mock_daos()->daos_kv_put(mock_daos()->get_mock_handle(), DAOS_TX_NONE, 0,
							 key, value_size, value, event);
  }

  virtual void read_raw(const char* key, char* buffer, size_t* size,
						daos_event_t* event = NULL) {
	mock_daos()->daos_kv_get(mock_daos()->get_mock_handle(), DAOS_TX_NONE, 0,
							 key, size, buffer, NULL);
  }
};