#ifndef __INCLUDED_DA_UTIL_LOGGING_WRAPPER_H_
#define __INCLUDED_DA_UTIL_LOGGING_WRAPPER_H_

#include <iostream>
#include <sstream>

namespace da {
namespace util {
namespace {

static void LogRecursive(const char* file, const char* function, int line,
                         std::ostringstream& msg) {
  std::cerr << file << " in function " << function << " at line " << line
            << ": " << msg.str() << std::endl;
}

template <typename T, typename... Args>
static void LogRecursive(const char* file, const char* function, int line,
                         std::ostringstream& msg, T value,
                         const Args&... args) {
  msg << value;
  LogRecursive(file, function, line, msg, args...);
}

}  // namespace

template <typename... Args>
static void LogWrapper(const char* file, const char* function, int line,
                       const Args&... args) {
  std::ostringstream msg;
  LogRecursive(file, function, line, msg, args...);
}

}  // namespace util
}  // namespace da

#endif  // __INCLUDED_DA_UTIL_LOGGING_WRAPPER_H_
