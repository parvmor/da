#ifndef __INCLUDED_DA_BROADCAST_UNIFORM_RELIABLE_H_
#define __INCLUDED_DA_BROADCAST_UNIFORM_RELIABLE_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include <da/link/perfect_link.h>
#include <da/process/process.h>

namespace da {
namespace broadcast {

class UniformReliable {
 public:
  UniformReliable(
      process::Process* local_process,
      std::vector<std::unique_ptr<link::PerfectLink>>* perfect_links);

  void broadcast(int message);

  bool deliver(int process_id, std::string message);

 private:
  void rebroadcast(std::string);
  process::Process* local_process_;
  std::vector<std::unique_ptr<link::PerfectLink>>* perfect_links_;
  std::shared_timed_mutex mutex_;
  // Denotes the messages that have been received as keys and
  // the set of process_ids those messages have been receives from as values.
  std::unordered_map<std::string, std::unordered_set<int>> received_messages_;
  // Denotes the delivered messages
  std::unordered_set<std::string> delivered_messages_;
};

}  // namespace broadcast
}  // namespace da

#endif  // __INCLUDED_DA_BROADCAST_UNIFORM_RELIABLE_H_
