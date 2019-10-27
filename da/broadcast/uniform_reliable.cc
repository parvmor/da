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

UniformReliable::UniformReliable(
    process::Process* local_process,
    std::vector<std::unique_ptr<link::PerfectLink>>* perfect_links)
    : local_process_(local_process), perfect_links_(perfect_links) {}

void UniformReliable::broadcast(int message) {
  if (message < 1 || message > local_process_->getMessageCount()) {
    LOG("Tried to broadcast message with id ", message,
        " out of bounds. Skipping.");
    return;
  }
  for (const auto& perfect_link : *perfect_links_) {
    // TODO (wiedeflo): Maybe pass this to the executor?
    perfect_link->sendMessage(std::to_string(local_process_->getId()) + "~" +
                              std::to_string(message));
  }
}

void UniformReliable::rebroadcast(std::string msg) {
  for (const auto& perfect_link : *perfect_links_) {
    // TODO (wiedeflo): Maybe pass this to the executor?
    perfect_link->sendMessage(msg);
  }
}

bool UniformReliable::deliver(int process_id, std::string message) {
  {
    std::shared_lock<std::shared_timed_mutex> lock(mutex_);
    // Rebroadcast if the message is received for the first time.
    if (received_messages_.find(message) == received_messages_.end())
      rebroadcast(message);
  }
  {
    std::shared_lock<std::shared_timed_mutex> lock(mutex_);
    received_messages_[message].insert(process_id);
    if (delivered_messages_.find(message) == delivered_messages_.end() &&
        received_messages_[message].size() > perfect_links_->size() / 2) {
      // Deliver
      delivered_messages_.insert(message);
      LOG("URB delivered message ", message);
      return true;
    }
  }
  return false;
  /*if (process_id < 1 || process_id > int((*perfect_links_).size())) {
    LOG("Received message: ", message,
        " from process with unknown id: ", process_id);
    return false;
  }
  if (!(*perfect_links_)[process_id - 1]->recvMessage(message)) {
    LOG("Message: ", message, " from process with id: ", process_id,
        " was rejected by perfect link.");
    return false;
  }*/
}

}  // namespace broadcast
}  // namespace da
