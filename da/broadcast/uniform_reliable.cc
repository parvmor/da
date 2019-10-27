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
int unpackProcessId(const std::string& msg) {
  return util::stringToInteger(msg.data());
}

}  // namespace

const int urb_min_length = link::min_length + sizeof(int);

UniformReliable::UniformReliable(
    const process::Process* local_process,
    std::vector<std::unique_ptr<link::PerfectLink>> perfect_links)
    : local_process_(local_process), perfect_links_(std::move(perfect_links)) {}

int UniformReliable::constructIdentity(const std::string* msg) {
  using namespace std::string_literals;
  std::string broadcast_msg = ""s;
  broadcast_msg += util::integerToString(local_process_->getId());
  broadcast_msg += *msg;
  return identity_manager_.assignId(broadcast_msg);
}

bool UniformReliable::deliverToPerfectLink(const std::string& msg,
                                           int process_id) {
  if (process_id < 1 || process_id > int(perfect_links_.size())) {
    LOG("Received message: ", util::stringToBinary(&msg),
        " from process with unknown id: ", process_id);
    return false;
  }
  if (!perfect_links_[process_id - 1]->recvMessage(msg)) {
    LOG("Message: ", util::stringToBinary(&msg),
        " from process with id: ", process_id,
        " was rejected by perfect link.");
    return false;
  }
  return true;
}

void UniformReliable::broadcast(const std::string* msg) {
  int id = constructIdentity(msg);
  {
    std::unique_lock<std::shared_timed_mutex> lock(mutex_);
    received_messages_[id] = {};
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
  // Find the perfect link and deliver at the level of perfect link first.
  int process_id = unpackProcessId(msg);
  deliverToPerfectLink(msg, process_id);
  // Now deliver at the level of uniform reliable broadcast.
  std::string broadcast_msg(msg.data() + link::min_length,
                            msg.size() - link::min_length);
  int id = identity_manager_.assignId(broadcast_msg);
  // Update the process id to that of unifrom reliable.
  process_id = unpackProcessId(broadcast_msg);
  {
    std::unique_lock<std::shared_timed_mutex> lock(mutex_);
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
      LOG("URB delivered the message: ", util::stringToBinary(&broadcast_msg),
          " ", util::stringToInteger(broadcast_msg.data() + sizeof(int)));
      return true;
    }
  }
  return false;
}

}  // namespace broadcast
}  // namespace da
