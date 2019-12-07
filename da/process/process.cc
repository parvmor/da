#include <da/process/process.h>

#include <iostream>
#include <ostream>

namespace da {
namespace process {

void Process::printDependencies() const {
  std::cerr << "{ ";
  for (const auto& dependency : dependencies_) {
    std::cout << dependency << " ";
  }
  std::cout << " }\n";
}

std::ostream& operator<<(std::ostream& stream, const Process& process) {
  return stream << "Process{ Id: " << process.id_
                << ", IP Addr: " << process.ip_addr_
                << ", Port: " << process.port_
                << ", Messages: " << process.message_count_
                << ", IsCurrent: " << process.current_ << " }";
}

}  // namespace process
}  // namespace da
