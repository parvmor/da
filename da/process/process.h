#ifndef __INCLUDED_DA_PROCESS_PROCESS_H_
#define __INCLUDED_DA_PROCESS_PROCESS_H_

#include "stdint.h"
#include "string"

namespace da {
namespace process {

class Process {
 public:
  Process(uint32_t id, std::string ip_addr, uint16_t port, bool current = false)
      : id_(id), ip_addr_(std::move(ip_addr)), port_(port), current_(current) {}

  uint32_t getId() { return id_; }

  uint16_t getPort() { return port_; }

  const std::string& getIPAddr() { return ip_addr_; }

  bool isCurrent() { return current_; }

 private:
  const uint32_t id_;
  const std::string ip_addr_;
  const uint16_t port_;
  // Denotes if this process is the one that is actually running.
  const bool current_;
};

}  // namespace process
}  // namespace da

#endif  // __INCLUDED_DA_PROCESS_PROCESS_H_
