#ifndef MK_ERRORS_H
#define MK_ERRORS_H

#include <stdexcept>

#include <daos.h>
#include <daos_errno.h>

class DaosError;

struct Position {
	Position(std::string func) : func(func) {}
	int         line;
	std::string file;
	std::string func;
};

#define DAOS_CHECK(code)                                                                           \
	do {                                                                                       \
		if (code != 0) {                                                                   \
			throw DaosError(code, Position(__func__));                                 \
		}                                                                                  \
	} while (0)

class DaosError : virtual public std::runtime_error
{
      public:
	DaosError(int error_code, Position position)
	    : std::runtime_error(d_errstr(error_code)), error_code_(error_code)
	{
	}

	int get_error_code() const { return error_code_; }

      private:
	int error_code_;
};

#endif // !MK_ERRORS_H