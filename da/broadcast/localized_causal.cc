#include <da/broadcast/localized_causal.h>

#include <algorithm>
#include <deque>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>

#include <da/broadcast/uniform_reliable.h>
#include <da/da_proc.h>
#include <da/process/process.h>
#include <da/util/logging.h>
#include <da/util/util.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

namespace da {
namespace broadcast {
namespace {

// Assumes that the message has a valid minimum length.
inline int unpackProcessId(const std::string& msg) {
  return util::stringToInteger<uint16_t>(msg.data());
}

// Assumes that the message has a valid minimum length.
inline std::vector<int> unpackVectorClock(const std::string& msg,
                                          int no_of_dependencies) {
  std::vector<int> dependencies(no_of_dependencies);
  for (int i = 0; i < no_of_dependencies; i++) {
    dependencies[i] = util::stringToInteger<int>(msg.data() + sizeof(uint16_t) +
                                                 i * sizeof(int));
  }
  return dependencies;
}

inline int unpackMessage(const std::string& msg, int no_of_dependencies) {
  return util::stringToInteger<int>(msg.data() + sizeof(uint16_t) +
                                    no_of_dependencies * sizeof(int));
}

}  // namespace

int lcb_min_length = urb_min_length + sizeof(uint16_t) + 2 * sizeof(int);
// This value is overwritten in da_proc.cc
int lcb_max_length = urb_min_length + sizeof(uint16_t) + 2 * sizeof(int);

UniformLocalizedCausal::UniformLocalizedCausal(
    const process::Process* local_process, std::unique_ptr<UniformReliable> urb,
    std::vector<std::unique_ptr<process::Process>> processes,
    spdlog::logger* file_logger)
    : local_process_(local_process),
      urb_(std::move(urb)),
      processes_(std::move(processes)),
      file_logger_(file_logger),
      broadcast_msgs_(0) {
  process_to_messages_.reserve(processes_.size());
  vector_clock_.assign(processes_.size(), 0);
}

int UniformLocalizedCausal::constructIdentity(const std::string* msg) {
  using namespace std::string_literals;
  std::string broadcast_msg = ""s;
  broadcast_msg += util::integerToString<uint16_t>(local_process_->getId());
  for (const auto& dependency : local_process_->getDependencies()) {
    // If this is the local process we cannot increment the vector clock else
    // FIFO order will be violated. Instead encode the sequence number of the
    // broadcasted message.
    if (dependency == local_process_->getId()) {
      broadcast_msg += util::integerToString<int>(broadcast_msgs_);
    } else {
      broadcast_msg += util::integerToString<int>(vector_clock_[dependency]);
    }
  }
  broadcast_msg += *msg;
  return identity_manager_.assignId(broadcast_msg);
}

bool UniformLocalizedCausal::deliverToURB(const std::string& msg) {
  if (!urb_->deliver(msg)) {
    LOG("Message: ", util::stringToBinary(&msg), " was rejected by URB.");
    return false;
  }
  return true;
}

inline bool UniformLocalizedCausal::shouldBroadcast() {
  int num_threads = std::thread::hardware_concurrency();
  int unreceived_msgs = broadcast_msgs_ - delivered_msgs_;
  if (unreceived_msgs > 1500 * std::min(num_threads, 6)) {
    return false;
  }
  return true;
}

void UniformLocalizedCausal::broadcast(const std::string* msg) {
  // Busy wait until we are allowed to broadcast. This helps in reducing the
  // traffic in the network. Reasoning is that if we have a lot of messages
  // pending to be delivered we are not doing anything good by broadcasting new
  // messages.
  while (!shouldBroadcast()) {
    // Sleep for 1 milli second(s).
    util::nanosleep(1000000);
    if (kCanStop) {
      return;
    }
  }
  int id;
  {
    // Need to take on the lock since we are reading vector clock while
    // constructing identity.
    std::unique_lock<std::mutex> lock(mutex_);
    id = constructIdentity(msg);
    file_logger_->info("b {}", da::util::stringToInteger<int>(*msg));
  }
  broadcast_msgs_ += 1;
  urb_->broadcast(identity_manager_.getValue(id));
}

void UniformLocalizedCausal::triggerDeliveries(int init_process_id) {
  std::queue<int> msg_queue;
  // `enqueue_msgs` enqueues all messages that were pending for `process_id`'s
  // vector clock to become the current value.
  auto enqueue_msgs = [&](int process_id) {
    const auto it =
        process_to_messages_[process_id].find(vector_clock_[process_id]);
    if (it == process_to_messages_[process_id].end()) {
      return;
    }
    for (const auto& msg_id : it->second) {
      msg_queue.push(msg_id);
    }
    // We no longer need to maintain the msg_id's for the current value of
    // vector clock of the process.
    process_to_messages_[process_id].erase(it);
  };
  // Schedule all the messages were waiting for an update in `init_process_id`'s
  // vector clock.
  enqueue_msgs(init_process_id);
  // The messages in `msg_queue` can either be deliverable (they have all their
  // dependencies satisfied) or non-deliverable (in which case we need to find
  // another watched index for the message).
  while (!msg_queue.empty()) {
    int msg_id = msg_queue.front();
    msg_queue.pop();
    bool can_deliver = true;
    // Find the first dependency that hasn't been fullfilled for this message.
    // This will eliminate all expired dependencies that might have been
    // satisfied because we watching only one dependency.
    while (!message_to_dependencies_[msg_id].empty()) {
      const auto& watched_index = message_to_dependencies_[msg_id].front();
      // Continue if this dependency has already been satisfied.
      if (vector_clock_[watched_index.first] >= watched_index.second) {
        message_to_dependencies_[msg_id].pop_front();
        continue;
      }
      // If this dependency is not satisfied we cannot deliver the message yet.
      // We update the message's watched index to point to this dependency.
      can_deliver = false;
      process_to_messages_[watched_index.first][watched_index.second].push_back(
          msg_id);
      message_to_dependencies_[msg_id].pop_front();
      break;
    }
    if (can_deliver) {
      // Clear the memory used by msg_id as the message can be delivered.
      message_to_dependencies_.erase(msg_id);
      // Find the sneder and content of the message.
      const std::string* msg = identity_manager_.getValue(msg_id);
      int process_id = unpackProcessId(*msg);
      int no_of_dependencies = processes_[process_id]->getDependencies().size();
      file_logger_->info("d {} {}", process_id + 1,
                         unpackMessage(*msg, no_of_dependencies));
      // Update account keeping for delivered messages if this is a local
      // message.
      if (process_id == local_process_->getId()) {
        delivered_msgs_ += 1;
      }
      // Update the vector clock for the sender and enqueue all messages that
      // were waiting for this update to happen.
      vector_clock_[process_id] += 1;
      enqueue_msgs(process_id);
    }
  }
  return;
}

bool UniformLocalizedCausal::deliver(const std::string& msg) {
  // First we need the lower level of abstraction, i.e., URB to accept the
  // message. Only then can we deliver to LCB.
  if (!deliverToURB(msg)) {
    return false;
  }
  // Recover the message broadcasted by the LCB abstraction.
  std::string broadcast_msg(msg.data() + urb_min_length,
                            msg.size() - urb_min_length);
  // Find the id, sender and dependencies of this message.
  int id = identity_manager_.assignId(broadcast_msg);
  int process_id = unpackProcessId(broadcast_msg);
  const auto& dependencies = processes_[process_id]->getDependencies();
  std::vector<int> msg_vector_clock =
      unpackVectorClock(broadcast_msg, dependencies.size());
  std::unique_lock<std::mutex> lock(mutex_);
  // Deliver the message if all its dependencies are satisfied.
  bool can_deliver = true;
  for (int i = 0; i < int(dependencies.size()); i++) {
    if (vector_clock_[dependencies[i]] < msg_vector_clock[i]) {
      can_deliver = false;
      break;
    }
  }
  if (!can_deliver) {
    // Construct the positive differential values that are blocking the
    // delivery and store this dependencies in a map so when these dependencies
    // are satisfied we can trigger the delivery of this message.
    for (int i = 0; i < int(dependencies.size()); i++) {
      if (vector_clock_[dependencies[i]] >= msg_vector_clock[i]) {
        continue;
      }
      message_to_dependencies_[id].push_front(
          {dependencies[i], msg_vector_clock[i]});
    }
    // Here, watched index contains the number of dependencies required from the
    // watched process to deliver the message. We only need to watch one process
    // for any message at a time since, to have all dependencies of the message
    // satisfied we also would need to satisfy the watched process's dependency.

    // Randomly shuffle the dependencies so that the watched indices are
    // uniformly distributed.
    std::random_shuffle(message_to_dependencies_[id].begin(),
                        message_to_dependencies_[id].end());
    const auto& watched_index = message_to_dependencies_[id].front();
    // Maintain the mapping from watched index to message id.
    process_to_messages_[watched_index.first][watched_index.second].push_back(
        id);
    message_to_dependencies_[id].pop_front();
    return true;
  }
  // We can deliver this message!
  file_logger_->info("d {} {}", process_id + 1,
                     unpackMessage(broadcast_msg, dependencies.size()));
  // Update the account keeping of delivered messages for heurisitc.
  if (process_id == local_process_->getId()) {
    delivered_msgs_ += 1;
  }
  // Update vector clock and deliver the messages that were being blocked by
  // this message.
  vector_clock_[process_id] += 1;
  triggerDeliveries(process_id);
  return true;
}

}  // namespace broadcast
}  // namespace da
