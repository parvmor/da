#ifndef __INCLUDED_DA_LINK_PERFECT_LINK_H_
#define __INCLUDED_DA_LINK_PERFECT_LINK_H_

#include <da/executor/scheduler.h>
#include <da/process/process.h>
#include <da/socket/udp_socket.h>
#include <da/util/identity_manager.h>
#include <da/util/status.h>
#include <da/util/statusor.h>

#include <chrono>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace da {
namespace link {

extern const int max_length;
extern const int min_length;

class PerfectLink {
 public:
  PerfectLink(executor::Scheduler* scheduler, socket::UDPSocket* sock,
              const process::Process* local_process,
              const process::Process* foreign_process);

  PerfectLink(executor::Scheduler* scheduler, socket::UDPSocket* sock,
              const process::Process* local_process,
              const process::Process* foreign_process,
              std::chrono::microseconds interval);

  ~PerfectLink();

  // Sends a message containing the given message id to the foreign process.
  void sendMessage(const std::string* msg);

  // Receives the provided message at the level of perfect links.
  bool recvMessage(const std::string& msg);

 private:
  // Schedules a callback to send message periodically until an ack is
  // received.
  void sendMessageCallback(int id);

  // Sends an acknowledgement when of the received message.
  void ackMessage(const std::string& msg);

  // Constructs and returns a unique identity for the given message.
  int constructIdentity(const std::string* msg);

  executor::Scheduler* scheduler_;
  socket::UDPSocket* sock_;
  const process::Process* local_process_;
  const process::Process* foreign_process_;
  const std::chrono::microseconds interval_;
  std::shared_timed_mutex mutex_;
  // Used to assign a unique identity to messages at the current layer.
  util::IdentityManager<std::string> identity_manager_;
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
