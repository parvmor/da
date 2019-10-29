#ifndef __INCLUDED_DA_UTIL_LOGGING_H_
#define __INCLUDED_DA_UTIL_LOGGING_H_

#ifdef ENABLE_LOG
#include <da/util/logging_wrapper.h>

#define LOG(...) \
  da::util::LogWrapper(__FILE__, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)

#else  // ENABLE_LOG
#define LOG(...)
#endif  // ENABLE_LOG

#ifdef ENABLE_DEBUG
#include <da/util/logging_wrapper.h>

#define DEBUG(...) \
  da::util::LogWrapper(__FILE__, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)

#else  // ENABLE_DEBUG
#define DEBUG(...)
#endif  // ENABLE_DEBUG

#endif  // __INCLUDED_DA_UTIL_LOGGING_H_
