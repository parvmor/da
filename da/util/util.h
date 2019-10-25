#ifndef __INCLUDED_DA_UTIL_UTIL_H_
#define __INCLUDED_DA_UTIL_UTIL_H_

#include <string>

namespace da {
namespace util {

std::string integerToString(int x);

int stringToInteger(const char* buffer);

int stringToInteger(std::string str);

}  // namespace util
}  // namespace da

#endif  // __INCLUDED_DA_UTIL_UTIL_H_
