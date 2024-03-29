#include <da/broadcast/uniform_reliable.h>

#include <memory>
#include <string>
#include <vector>

#include <da/link/perfect_link.h>
#include <da/process/process.h>
#include <da/util/logging.h>
#include <da/util/util.h>

namespace da {
namespace broadcast {
namespace {

// Assumes that the message has a valid minimum length.
inline int unpackProcessId(const std::string& msg) {
  return util::stringToInteger<uint16_t>(msg.data());
}

}  // namespace

const int urb_min_length = link::min_length;

UniformReliable::UniformReliable(
    const process::Process* local_process,
    std::vector<std::unique_ptr<link::PerfectLink>> perfect_links)
    : local_process_(local_process), perfect_links_(std::move(perfect_links)) {}

int UniformReliable::constructIdentity(const std::string* msg) {
  using namespace std::string_literals;
  std::string broadcast_msg = ""s;
  broadcast_msg += *msg;
  return identity_manager_.assignId(broadcast_msg);
}

bool UniformReliable::deliverToPerfectLink(const std::string& msg) {
  int process_id = unpackProcessId(msg);
  if (process_id < 0 || process_id >= int(perfect_links_.size())) {
    LOG("Received message: ", util::stringToBinary(&msg),
        " from process with unknown id: ", process_id + 1);
    return false;
  }
  if (!perfect_links_[process_id]->recvMessage(msg)) {
    LOG("Message: ", util::stringToBinary(&msg),
        " from process with id: ", process_id + 1,
        " was rejected by perfect link.");
    return false;
  }
  return true;
}

void UniformReliable::broadcast(const std::string* msg) {
  int id = constructIdentity(msg);
  {
    std::unique_lock<std::mutex> lock(mutex_);
    if (received_messages_.find(id) == received_messages_.end()) {
      received_messages_[id] = {};
    }
  }
  const std::string* broadcast_msg = identity_manager_.getValue(id);
  for (const auto& perfect_link : perfect_links_) {
    perfect_link->sendMessage(broadcast_msg);
  }
}

void UniformReliable::rebroadcast(const std::string* msg) {
  for (const auto& perfect_link : perfect_links_) {
    perfect_link->sendMessage(msg);
  }
}

bool UniformReliable::canDeliver(int id) {
  if (received_messages_[id].size() <= perfect_links_.size() / 2) {
    return false;
  }
  if (delivered_messages_.find(id) != delivered_messages_.end()) {
    return false;
  }
  return true;
}

bool UniformReliable::deliver(const std::string& msg) {
  if (!deliverToPerfectLink(msg)) {
    return false;
  }
  // Now deliver at the level of uniform reliable broadcast.
  int process_id = unpackProcessId(msg);
  std::string broadcast_msg(msg.data() + link::min_length,
                            msg.size() - link::min_length);
  int id = identity_manager_.assignId(broadcast_msg);
  {
    std::unique_lock<std::mutex> lock(mutex_);
    // Rebroadcast the message if received for the first time.
    if (received_messages_.find(id) == received_messages_.end()) {
      received_messages_[id] = {};
      // We can unlock the mutex since, re-broadcasting might take some time and
      // we have already added id into the set of received messages.
      lock.unlock();
      rebroadcast(&broadcast_msg);
      // Acquire the lock once again.
      lock.lock();
    }
    received_messages_[id].insert(process_id);
    if (canDeliver(id)) {
      delivered_messages_.insert(id);
      LOG("URB delivered the message: ", util::stringToBinary(&broadcast_msg));
      return true;
    }
  }
  return false;
}

}  // namespace broadcast
}  // namespace da
