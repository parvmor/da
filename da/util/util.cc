#include <da/util/util.h>

#include <string>

namespace da {
namespace util {

std::string integerToString(int x) {
  std::string str;
  str.reserve(sizeof(int));
  for (int i = 1; i <= int(sizeof(int)); i++) {
    str += static_cast<char>((x >> (8 * (sizeof(int) - i))) & 0xFF);
  }
  return str;
}

int stringToInteger(const char* buffer) {
  int x = 0;
  for (int i = 1; i <= int(sizeof(int)); i++) {
    // Clear off any sign extension that might have happened.
    int byte = static_cast<int>(buffer[i - 1]) & 0xFF;
    byte <<= (8 * (sizeof(int) - i));
    x |= byte;
  }
  return x;
}

int stringToInteger(std::string str) { return stringToInteger(str.data()); }

}  // namespace util
}  // namespace da
