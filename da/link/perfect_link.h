#ifndef __INCLUDED_DA_LINK_PERFECT_LINK_H_
#define __INCLUDED_DA_LINK_PERFECT_LINK_H_

#include <chrono>
#include <functional>
#include <shared_mutex>
#include <unordered_set>

#include <da/executor/executor.h>
#include <da/process/process.h>
#include <da/socket/udp_socket.h>
#include <da/util/status.h>
#include <da/util/statusor.h>

namespace da {
namespace link {

extern const int msg_length;

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

  // Sends a message containing the given message id to the foreign process.
  void sendMessage(int message);

  // Receives the given message from the foreign process.
  //
  // TODO(parvmor): Send an acknowledgment of receiving back.
  bool recvMessage(int message);

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