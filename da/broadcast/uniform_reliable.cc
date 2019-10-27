#include <da/broadcast/uniform_reliable.h>

#include <memory>
#include <string>
#include <vector>

#include <da/link/perfect_link.h>
#include <da/message/message.h>
#include <da/process/process.h>
#include <da/util/logging.h>
#include <da/util/util.h>

namespace da {
namespace broadcast {

UniformReliable::UniformReliable(
    const process::Process* local_process,
    std::vector<std::unique_ptr<link::PerfectLink>> perfect_links)
    : local_process_(local_process), perfect_links_(std::move(perfect_links)) {}

void UniformReliable::broadcast(std::shared_ptr<std::string> message) {
  if (message == nullptr) {
    LOG("Received a nullptr as message. Skipping.");
    return;
  }
  for (const auto& perfect_link : perfect_links_) {
    perfect_link->sendMessage(message);
  }
}

bool UniformReliable::deliver(std::shared_ptr<char[]> message) {
  const auto msg = std::make_unique<message::Message>(message.get());
  int process_id = msg->getProcessId();
  if (process_id < 1 || process_id > int(perfect_links_.size())) {
    LOG("Received message: ", *msg,
        " from process with unknown id: ", process_id);
    return false;
  }
  int message_id = msg->getMessageId();
  if (message_id < 1 || message_id > local_process_->getMessageCount()) {
    LOG("Received an unknown message id: ", message_id,
        " from the process id: ", process_id, " by process ", *local_process_);
    return false;
  }
  if (!perfect_links_[process_id - 1]->recvMessage(msg.get())) {
    LOG("Message: ", *msg, " from process with id: ", process_id,
        " was rejected by perfect link.");
    return false;
  }
  LOG("Received message '", *msg, "'");
  return true;
}

}  // namespace broadcast
}  // namespace da
