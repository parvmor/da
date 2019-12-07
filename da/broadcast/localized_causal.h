#ifndef __INCLUDED_DA_BROADCAST_LOCALIZED_CAUSAL_H_
#define __INCLUDED_DA_BROADCAST_LOCALIZED_CAUSAL_H_

#include <deque>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <da/broadcast/uniform_reliable.h>
#include <da/process/process.h>
#include <da/util/util.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

namespace da {
namespace broadcast {

extern int lcb_min_length;
extern int lcb_max_length;

class UniformLocalizedCausal {
 public:
  UniformLocalizedCausal(
      const process::Process* local_process,
      std::unique_ptr<UniformReliable> urb,
      std::vector<std::unique_ptr<process::Process>> processes,
      spdlog::logger* file_logger);

  void broadcast(const std::string* msg);

  bool deliver(const std::string& msg);

 private:
  // Constructs and returns a unique identity for the given message.
  int constructIdentity(const std::string* msg);

  // Triggers the uniform reliable's delivery.
  bool deliverToURB(const std::string& msg);

  // Uses a heuristic to decide if we should stop broadcasting messages for a
  // while.
  bool shouldBroadcast();

  // Updates the watched indices of messages that were dependent on this process
  // id. Might deliver a message if there is no more index to be watched.
  void triggerDeliveries(int init_process_id);

  const process::Process* local_process_;
  std::unique_ptr<UniformReliable> urb_;
  std::vector<std::unique_ptr<process::Process>> processes_;
  // Used to log into the file in the requried format.
  spdlog::logger* file_logger_;
  // Used to assign a unique identity to the messages.
  util::IdentityManager<std::string> identity_manager_;

  std::mutex mutex_;
  // A map from process id to number of messages that you need to deliver the
  // message with id process_to_messages_[process id][number of messages].
  std::unordered_map<int, std::unordered_map<int, std::vector<int>>>
      process_to_messages_;
  // A local vector clock for this process.
  std::vector<int> vector_clock_;
  // A map from message id to it's corresponding vector clock values that are
  // not satisfied against local vector clock.
  std::unordered_map<int, std::deque<std::pair<int, int>>>
      message_to_dependencies_;
};

}  // namespace broadcast
}  // namespace da

#endif  // __INCLUDED_DA_BROADCAST_LOCALIZED_CAUSAL_H_
