#include <da/process/process.h>

#include <ostream>

namespace da {
namespace process {

std::ostream& operator<<(std::ostream& stream, const Process& process) {
  return stream << "Process{ Id: " << process.id_
                << ", IP Addr: " << process.ip_addr_
                << ", Port: " << process.port_
                << ", Messages: " << process.message_count_
                << ", IsCurrent: " << process.current_ << " }";
}

}  // namespace process
}  // namespace da
