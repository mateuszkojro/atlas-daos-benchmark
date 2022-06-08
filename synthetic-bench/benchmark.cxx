#include "Pool.h"
#include "daos.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
const std::string WHITESPACE = " \n\r\t\f\v";

std::string ltrim(const std::string& s) {
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

std::string rtrim(const std::string& s) {
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(const std::string& s) { return rtrim(ltrim(s)); }

int main() {
  std::fstream file("/tmp/containers.txt", std::ios::in);
  daos_init();
  Pool pool("mkojro");
  if (file.bad()) {
	std::cout << "Failed to open file: " << strerror(errno) << "\n";
	return 1;
  }

  std::string line;
  do {
	std::getline(file, line);
	line = trim(line);
	std::clog << "Removing: '" << line << "'\n";
	pool.remove_container(line);
  } while (line != "");

  pool.clean_up();
  daos_fini();
}
