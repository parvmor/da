#ifndef __INCLUDED_DA_UTIL_UTIL_H_
#define __INCLUDED_DA_UTIL_UTIL_H_

#include <unistd.h>
#include <string>

namespace da {
namespace util {

std::string integerToString(int x);

int stringToInteger(const char* buffer);

int stringToInteger(const std::string& str);

std::string boolToString(bool b);

bool stringToBool(const char* buffer);

bool stringToBool(const std::string& str);

void nanosleep(uint64_t time);

std::string stringToBinary(const std::string* str);

}  // namespace util
}  // namespace da

#endif  // __INCLUDED_DA_UTIL_UTIL_H_
