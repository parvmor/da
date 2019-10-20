#ifndef __INCLUDED_DA_LINK_PERFECT_LINK_H_
#define __INCLUDED_DA_LINK_PERFECT_LINK_H_

#include <chrono>
#include <shared_mutex>
#include <unordered_set>

#include <da/executor/executor.h>
#include <da/process/process.h>
#include <da/socket/udp_socket.h>
#include <da/util/status.h>
#include <da/util/statusor.h>

namespace da {
namespace link {

class PerfectLink {
 public:
  PerfectLink(executor::Executor* executor, socket::UDPSocket* sock,
              const process::Process* local_process,
              const process::Process* foreign_process);

  PerfectLink(executor::Executor*, socket::UDPSocket* sock,
              const process::Process* local_process,
              const process::Process* foreign_process,
              std::chrono::microseconds interval);

  ~PerfectLink();

  // Messages are always an integer as required by the description.
  void sendMessage(int message);

 private:
  void sendMessageCallback(int message);

  executor::Executor* executor_;
  socket::UDPSocket* sock_;
  const process::Process* local_process_;
  const process::Process* foreign_process_;
  const std::chrono::microseconds interval_;
  std::shared_timed_mutex mutex_;
  // Denotes the set of messages sent to foreign process that have not been
  // acknowledged yet.
  std::unordered_set<int> undelivered_messages_;
  // Denotes the set of messages received from foreign process that have not
  // been acknowledged yet.
  std::unordered_set<int> delivered_messages_;
};

}  // namespace link
}  // namespace da

#endif  // __INCLUDED_DA_LINK_PERFECT_LINK_H_
