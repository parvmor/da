#ifndef __INCLUDED_DA_PROCESS_PROCESS_H_
#define __INCLUDED_DA_PROCESS_PROCESS_H_

#include <algorithm>
#include <ostream>
#include <string>
#include <vector>

namespace da {
namespace process {

class Process {
 public:
  Process(int id, std::string ip_addr, int port, int message_count,
          bool current = false)
      : id_(id),
        ip_addr_(std::move(ip_addr)),
        port_(port),
        message_count_(message_count),
        current_(current) {}

  int getId() const { return id_; }

  const std::string& getIPAddr() const { return ip_addr_; }

  int getPort() const { return port_; }

  int getMessageCount() const { return message_count_; }

  bool isCurrent() const { return current_; }

  void addDependency(int id) { dependencies_.push_back(id); }

  void finalizeDependencies() {
    std::sort(dependencies_.begin(), dependencies_.end());
    dependencies_.erase(std::unique(dependencies_.begin(), dependencies_.end()),
                        dependencies_.end());
  }

  const std::vector<int>& getDependencies() const { return dependencies_; }

  void printDependencies() const;

 private:
  friend std::ostream& operator<<(std::ostream&, const Process&);

  const int id_;
  const std::string ip_addr_;
  const int port_;
  // Denotes the number of messages this process has to broadcast.
  const int message_count_;
  // Denotes if this process is the one that is actually running.
  const bool current_;
  // Denotes the process ids this process is dependent on.
  std::vector<int> dependencies_;
};

}  // namespace process
}  // namespace da

#endif  // __INCLUDED_DA_PROCESS_PROCESS_H_
