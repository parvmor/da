#include <da/broadcast/uniform_reliable.h>

#include <memory>
#include <vector>

#include <da/link/perfect_link.h>
#include <da/process/process.h>
#include <da/util/logging.h>

namespace da {
namespace broadcast {

UniformReliable::UniformReliable(
    process::Process* local_process,
    std::vector<std::unique_ptr<link::PerfectLink>> perfect_links)
    : local_process_(local_process), perfect_links_(std::move(perfect_links)) {}

void UniformReliable::broadcast(int message) {
  if (message < 1 || message > local_process_->getMessageCount()) {
    LOG("Tried to broadcast message with id ", message,
        " out of bounds. Skipping.");
    return;
  }
  for (const auto& perfect_link : perfect_links_) {
    perfect_link->sendMessage(message);
  }
}

bool UniformReliable::deliver(int process_id, int message) {
  if (process_id < 1 || process_id > int(perfect_links_.size())) {
    LOG("Received message: ", message, " from process with unknown id: ",
        process_id);
    return false;
  }
  if (!perfect_links_[process_id - 1]->recvMessage(message)) {
    LOG("Message: ", message, " from process with id: ", process_id,
        " was rejected by perfect link.");
    return false;
  }
  return true;
}

}  // namespace broadcast
}  // namespace da
