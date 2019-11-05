#include <da/util/util.h>
#include <time.h>

#include <bitset>
#include <string>

namespace da {
namespace util {

std::string integerToString(int x) {
  using namespace std::string_literals;
  std::string str = ""s;
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

int stringToInteger(const std::string& str) {
  return stringToInteger(str.data());
}

std::string boolToString(bool b) {
  using namespace std::string_literals;
  std::string str = ""s;
  if (b) {
    str += static_cast<char>(1);
  } else {
    str += static_cast<char>(0);
  }
  return str;
}

bool stringToBool(const char* buffer) {
  if (buffer[0] == '\0') {
    return false;
  }
  return true;
}

bool stringToBool(const std::string& str) { return stringToBool(str.data()); }

void nanosleep(uint64_t time) {
  struct timespec sleep_time;
  sleep_time.tv_sec = 0;
  sleep_time.tv_nsec = time;
  ::nanosleep(&sleep_time, nullptr);
}

std::string stringToBinary(const std::string* str) {
  std::string binary = "";
  for (const auto& c : *str) {
    binary += std::bitset<8>(c).to_string();
  }
  return binary;
}

}  // namespace util
}  // namespace da
