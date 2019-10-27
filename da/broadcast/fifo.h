#ifndef __INCLUDED_DA_BROADCAST_FIFO_H_
#define __INCLUDED_DA_BROADCAST_FIFO_H_

#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <da/broadcast/uniform_reliable.h>
#include <da/process/process.h>
#include <da/util/util.h>

namespace da {
namespace broadcast {

extern const int fifo_min_length;

class UniformFIFOReliable {
 public:
  UniformFIFOReliable(const process::Process* local_process,
                      std::unique_ptr<UniformReliable> urb, int processes);

  void broadcast(const std::string* msg);

  bool deliver(const std::string& msg);

 private:
  // Constructs and returns a unique identity for the given message.
  int constructIdentity(const std::string* msg);

  // Triggers the uniform reliable's delivery.
  bool deliverToURB(const std::string& msg);

  const process::Process* local_process_;
  std::unique_ptr<UniformReliable> urb_;
  // Denotes the sequence number used to decide delivery order.
  std::atomic<int> lsn_;
  // Used to assign a unique identity to the messages.
  util::IdentityManager<std::string> identity_manager_;
  class ProcessData;
  std::vector<std::unique_ptr<ProcessData>> process_data_;
};

class UniformFIFOReliable::ProcessData {
 public:
  ProcessData(UniformFIFOReliable* fifo_urb);

  void deliver(int sn, int msg_id);

 private:
  UniformFIFOReliable* fifo_urb_;
  std::atomic<int> next_;
  std::mutex mutex_;
  // Contains the mapping from the sequence number of undelivered message to
  // message identifier.
  std::unordered_map<int, int> pending_messages_;
};

}  // namespace broadcast
}  // namespace da

#endif  // __INCLUDED_DA_BROADCAST_FIFO_H_
