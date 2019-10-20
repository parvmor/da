#ifndef __INCLUDED_DA_BROADCAST_UNIFORM_RELIABLE_H_
#define __INCLUDED_DA_BROADCAST_UNIFORM_RELIABLE_H_

#include <memory>
#include <vector>

#include <da/link/perfect_link.h>
#include <da/process/process.h>

namespace da {
namespace broadcast {

class UniformReliable {
 public:
  UniformReliable(
      process::Process* local_process,
      std::vector<std::unique_ptr<link::PerfectLink>> perfect_links);

  void broadcast(int message);

  bool deliver(int process_id, int message);

 private:
  process::Process* local_process_;
  std::vector<std::unique_ptr<link::PerfectLink>> perfect_links_;
};

}  // namespace broadcast
}  // namespace da

#endif  // __INCLUDED_DA_BROADCAST_UNIFORM_RELIABLE_H_
