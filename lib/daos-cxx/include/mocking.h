#include "daos.h"
#include "daos_errno.h"
#include "daos_types.h"
#include <cstdint>
#include <memory>
#include <ostream>
#include <stdexcept>

#ifndef MK_MOCK_H
#define MK_MOCK_H

// #define LOG_EVENT(x) __func__

enum MockDAOSOperationType
{
  UNDEFINED,
  POOL_CONNECT,
  CONT_CREATE,
  CONT_DESTROY,
  CONT_OPEN,
  CONT_CLOSE,
  KV_GET,
  KV_PUT,
  KV_OPEN
};

class IDAOSLog {
 public:
  virtual void log(const std::string&,
				   MockDAOSOperationType operation = UNDEFINED) = 0;
  virtual ~IDAOSLog() = default;
};

class DAOSSimpleLog : public IDAOSLog {
 public:
  DAOSSimpleLog(std::ostream& stream) : ostream_(stream) {}

  void log(const std::string& func_name,
		   MockDAOSOperationType operation = UNDEFINED) {
	ostream_ << func_name << "\n";
  }
  std::ostream& ostream_;
};

class MockDAOS {
 public:
  MockDAOS(IDAOSLog* log) : logger_(log) {}

  void daos_pool_connect_cpp() {
	logger_->log(__PRETTY_FUNCTION__, POOL_CONNECT);
  }

  void daos_cont_create_cpp() {
	logger_->log(__PRETTY_FUNCTION__, CONT_CREATE);


  
  }

  void daos_cont_destroy_cpp() {
	logger_->log(__PRETTY_FUNCTION__, CONT_DESTROY);
  }

  void daos_cont_open_cpp() { logger_->log(__PRETTY_FUNCTION__, CONT_OPEN); }

  void daos_cont_close() { logger_->log(__PRETTY_FUNCTION__, CONT_CLOSE); }

  void daos_kv_get() { logger_->log(__PRETTY_FUNCTION__, KV_GET); }

  void daos_kv_put() { logger_->log(__PRETTY_FUNCTION__, KV_OPEN); }

  void daos_kv_open() { logger_->log(__PRETTY_FUNCTION__, KV_GET); }

  /*
	  Full function signatures if more detailed logging would make sense
  */

  int daos_pool_connect_cpp(const char* pool, const char* sys,
							unsigned int flags, daos_handle_t* poh,
							daos_pool_info_t* info, daos_event_t* ev);

  int daos_cont_create_cpp(daos_handle_t poh, uuid_t* uuid,
						   daos_prop_t* cont_prop, daos_event_t* ev);

  int daos_cont_destroy_cpp(daos_handle_t poh, const char* cont, int force,
							daos_event_t* ev);

  int daos_cont_open_cpp(daos_handle_t poh, const char* cont,
						 unsigned int flags, daos_handle_t* coh,
						 daos_cont_info_t* info, daos_event_t* ev);

  int daos_cont_close(daos_handle_t coh, daos_event_t* ev);

  int daos_kv_get(daos_handle_t oh, daos_handle_t th, uint64_t flags,
				  const char* key, daos_size_t* size, void* buf,
				  daos_event_t* ev) {
	logger_->log(__PRETTY_FUNCTION__);
	return DER_SUCCESS;
  }

  int daos_kv_put(daos_handle_t oh, daos_handle_t th, uint64_t flags,
				  const char* key, daos_size_t size, const void* buf,
				  daos_event_t* ev) {
	logger_->log(__PRETTY_FUNCTION__);
	return DER_SUCCESS;
  }

  daos_handle_t get_mock_handle() {
	daos_handle_t mock_handle;
	mock_handle.cookie = 2137;
	return mock_handle;
  }

 private:
  IDAOSLog* logger_;
  // std::ostream& output_stream;
};

class MockDAOSObj {
 protected:
  MockDAOSObj(const std::weak_ptr<MockDAOS>& mock_daos)
	  : mock_daos_(mock_daos) {}

  std::shared_ptr<MockDAOS> mock_daos() { return mock_daos_.lock(); }

 private:
  std::weak_ptr<MockDAOS> mock_daos_;
};

#endif// !MK_MOCK_H