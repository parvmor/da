#include <da/broadcast/localized_causal.h>
#include <da/broadcast/uniform_reliable.h>
#include <da/da_proc.h>
#include <da/process/process.h>
#include <da/util/logging.h>
#include <da/util/util.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <deque>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>

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
      file_logger_(file_logger) {
  process_to_messages_.reserve(processes_.size());
  vector_clock_.assign(processes_.size(), 0);
}

int UniformLocalizedCausal::constructIdentity(const std::string* msg) {
  using namespace std::string_literals;
  std::string broadcast_msg = ""s;
  broadcast_msg += util::integerToString<uint16_t>(local_process_->getId());
  for (const auto& dependency : local_process_->getDependencies()) {
    broadcast_msg += util::integerToString<int>(vector_clock_[dependency]);
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

bool UniformLocalizedCausal::shouldBroadcast() { return true; }

void UniformLocalizedCausal::broadcast(const std::string* msg) {
  while (!shouldBroadcast()) {
    // Sleep for 1 milli second(s).
    util::nanosleep(1000000);
    if (kCanStop) {
      return;
    }
  }
  int id;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    id = constructIdentity(msg);
    file_logger_->info("b {}", da::util::stringToInteger<int>(*msg));
    file_logger_->info("d {} {}", local_process_->getId() + 1,
                       da::util::stringToInteger<int>(*msg));
    vector_clock_[local_process_->getId()] += 1;
  }
  urb_->broadcast(identity_manager_.getValue(id));
}

void UniformLocalizedCausal::triggerDeliveries(int init_process_id) {
  std::queue<int> msg_queue;
  auto enqueue_msgs = [&](int process_id) {
    const auto it =
        process_to_messages_[process_id].find(vector_clock_[process_id]);
    if (it == process_to_messages_[process_id].end()) {
      return;
    }
    for (const auto& msg_id : it->second) {
      msg_queue.push(msg_id);
    }
    process_to_messages_[process_id].erase(it);
  };
  enqueue_msgs(init_process_id);
  while (!msg_queue.empty()) {
    int msg_id = msg_queue.front();
    msg_queue.pop();
    bool can_deliver = true;
    while (!message_to_dependencies_[msg_id].empty()) {
      const auto& watched_index = message_to_dependencies_[msg_id].front();
      if (vector_clock_[watched_index.first] >= watched_index.second) {
        message_to_dependencies_[msg_id].pop_front();
        continue;
      }
      can_deliver = false;
      process_to_messages_[watched_index.first][watched_index.second].push_back(
          msg_id);
      message_to_dependencies_[msg_id].pop_front();
      break;
    }
    if (can_deliver) {
      message_to_dependencies_.erase(msg_id);
      const std::string* msg = identity_manager_.getValue(msg_id);
      int process_id = unpackProcessId(*msg);
      int no_of_dependencies = processes_[process_id]->getDependencies().size();
      file_logger_->info("d {} {}", unpackProcessId(*msg) + 1,
                         unpackMessage(*msg, no_of_dependencies));
      vector_clock_[process_id] += 1;
      enqueue_msgs(process_id);
      return;
    }
  }
}

bool UniformLocalizedCausal::deliver(const std::string& msg) {
  if (!deliverToURB(msg)) {
    return false;
  }
  // Now deliver at the level of Localized Causal Broadcast.
  std::string broadcast_msg(msg.data() + urb_min_length,
                            msg.size() - urb_min_length);
  int id = identity_manager_.assignId(broadcast_msg);
  int process_id = unpackProcessId(broadcast_msg);
  // Quickfix to enable delivery at the moment of broadcast
  if (process_id == local_process_->getId()) {
    return true;
  }
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
    // delivery.
    for (int i = 0; i < int(dependencies.size()); i++) {
      if (vector_clock_[dependencies[i]] >= msg_vector_clock[i]) {
        continue;
      }
      message_to_dependencies_[id].push_front(
          {dependencies[i], msg_vector_clock[i]});
    }
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
  vector_clock_[process_id] += 1;
  triggerDeliveries(process_id);
  return true;
}

}  // namespace broadcast
}  // namespace da
