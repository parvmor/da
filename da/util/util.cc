#include <da/util/util.h>

#include <time.h>
#include <bitset>
#include <string>

namespace da {
namespace util {

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
