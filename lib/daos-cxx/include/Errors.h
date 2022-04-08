#ifndef MK_ERRORS_H
#define MK_ERRORS_H

#include <stdexcept>

#include "daos.h"
#include <daos_errno.h>
#include <iostream>
#include <string>

#define RED "\u001b[31m"
#define RESET "\u001b[0m"
#define ERROR(msg) std::cerr << RED << msg << RESET << std::endl;

class DAOSError;

struct Position
{
  Position(std::string file, int line, std::string func)
	  : file(file), line(line), func(func) {}
  std::string file;
  int line;
  std::string func;
};

#define GET_CODE_INFO                                                          \
  { __FILE__, __LINE__, __PRETTY_FUNCTION__ }

#define DAOS_CHECK(command)                                                    \
  do {                                                                         \
	int code = command;                                                        \
	if (code != DER_SUCCESS) {                                                 \
	  throw DAOSError(code,                                                    \
					  Position(__FILE__, __LINE__, __PRETTY_FUNCTION__));      \
	}                                                                          \
  } while (0)

class DAOSError : virtual public std::runtime_error {
 public:
  DAOSError(int error_code, Position position)
	  : std::runtime_error(std::string(d_errstr(error_code)) + ": "
						   + std::string(d_errdesc(error_code)) + " at "
						   + position.file + ":" + std::to_string(position.line)
						   + " " + position.func),
		error_code_(error_code) {}

  int get_error_code() const { return error_code_; }

 private:
  int error_code_;
};

#endif// !MK_ERRORS_H