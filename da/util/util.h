#ifndef __INCLUDED_DA_UTIL_UTIL_H_
#define __INCLUDED_DA_UTIL_UTIL_H_

#include <unistd.h>
#include <string>

namespace da {
namespace util {

template <typename T>
std::string integerToString(T x) {
  using namespace std::string_literals;
  std::string str = ""s;
  str.reserve(sizeof(T));
  for (int i = 1; i <= int(sizeof(T)); i++) {
    str += static_cast<char>((x >> (8 * (sizeof(T) - i))) & 0xFF);
  }
  return str;
}

template <typename T>
T stringToInteger(const char* buffer) {
  T x = 0;
  for (int i = 1; i <= int(sizeof(T)); i++) {
    // Clear off any sign extension that might have happened.
    T byte = static_cast<T>(buffer[i - 1]) & 0xFF;
    byte <<= (8 * (sizeof(T) - i));
    x |= byte;
  }
  return x;
}

template <typename T>
T stringToInteger(const std::string& str) {
  return stringToInteger<T>(str.data());
}

std::string boolToString(bool b);

bool stringToBool(const char* buffer);

bool stringToBool(const std::string& str);

void nanosleep(uint64_t time);

std::string stringToBinary(const std::string* str);

}  // namespace util
}  // namespace da

#endif  // __INCLUDED_DA_UTIL_UTIL_H_
