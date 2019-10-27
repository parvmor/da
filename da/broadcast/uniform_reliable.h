#ifndef __INCLUDED_DA_BROADCAST_UNIFORM_RELIABLE_H_
#define __INCLUDED_DA_BROADCAST_UNIFORM_RELIABLE_H_

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <da/link/perfect_link.h>
#include <da/process/process.h>
#include <da/util/identity_manager.h>

namespace da {
namespace broadcast {

extern const int urb_min_length;

class UniformReliable {
 public:
  UniformReliable(
      const process::Process* local_process,
      std::vector<std::unique_ptr<link::PerfectLink>> perfect_links);

  void broadcast(const std::string* msg);

  bool deliver(const std::string& msg);

 private:
  // Constructs and returns a unique identity for the given message.
  int constructIdentity(const std::string* msg);

  // Triggers the perfect link delivery.
  bool deliverToPerfectLink(const std::string& msg);

  // Rebroadcasts the message received from someone.
  void rebroadcast(const std::string* msg);

  // Returns if the given message identifier is eligible to be delivered.
  bool canDeliver(int id);

  const process::Process* local_process_;
  std::vector<std::unique_ptr<link::PerfectLink>> perfect_links_;
  std::shared_timed_mutex mutex_;
  // Used to assign a unique identity to the messages.
  util::IdentityManager<std::string> identity_manager_;
  // A map from messages that have been received to the set of process_ids from
  // who the same message was received. Note that the key set of this map
  // denotes the set of pending messages too.
  std::unordered_map<int, std::unordered_set<int>> received_messages_;
  // Denotes the messages that have been delivered.
  std::unordered_set<int> delivered_messages_;
};

}  // namespace broadcast
}  // namespace da

#endif  // __INCLUDED_DA_BROADCAST_UNIFORM_RELIABLE_H_
