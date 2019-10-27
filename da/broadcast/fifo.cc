#include <da/broadcast/fifo.h>

#include <memory>
#include <string>

#include <da/broadcast/uniform_reliable.h>
#include <da/process/process.h>
#include <da/util/logging.h>
#include <da/util/util.h>

namespace da {
namespace broadcast {
namespace {

// Assumes that the message has a valid minimum length.
inline int unpackProcessId(const std::string& msg) {
  return util::stringToInteger(msg.data());
}

// Assumes that the message has a valid minimum length.
inline int unpackSn(const std::string& msg) {
  return util::stringToInteger(msg.data() + sizeof(int));
}

}  // namespace

const int fifo_min_length = urb_min_length + 3 * sizeof(int);

UniformFIFOReliable::UniformFIFOReliable(
    const process::Process* local_process, std::unique_ptr<UniformReliable> urb,
    int processes, std::shared_ptr<spdlog::logger> file_logger)
    : local_process_(local_process),
      urb_(std::move(urb)),
      lsn_(0),
      file_logger_(file_logger) {
  process_data_.reserve(processes);
  for (int i = 0; i < processes; i++) {
    process_data_.emplace_back(
        std::make_unique<ProcessData>(this, file_logger));
  }
}

int UniformFIFOReliable::constructIdentity(const std::string* msg) {
  using namespace std::string_literals;
  std::string broadcast_msg = ""s;
  broadcast_msg += util::integerToString(local_process_->getId());
  broadcast_msg += util::integerToString(lsn_++);
  broadcast_msg += *msg;
  return identity_manager_.assignId(broadcast_msg);
}

bool UniformFIFOReliable::deliverToURB(const std::string& msg) {
  if (!urb_->deliver(msg)) {
    LOG("Message: ", util::stringToBinary(&msg), " was rejected by URB.");
    return false;
  }
  return true;
}

void UniformFIFOReliable::broadcast(const std::string* msg) {
  int id = constructIdentity(msg);
  file_logger_->info("b {}", da::util::stringToInteger(*msg));
  urb_->broadcast(identity_manager_.getValue(id));
}

bool UniformFIFOReliable::deliver(const std::string& msg) {
  if (!deliverToURB(msg)) {
    return false;
  }
  // Now deliver at the level of FIFO broadcast.
  std::string broadcast_msg(msg.data() + urb_min_length,
                            msg.size() - urb_min_length);
  int id = identity_manager_.assignId(broadcast_msg);
  int process_id = unpackProcessId(broadcast_msg);
  int sn = unpackSn(broadcast_msg);
  if (process_id < 1 || process_id > int(process_data_.size())) {
    LOG("Received an irrelevant process id: ", process_id, ". Skipping.");
    return false;
  }
  process_data_[process_id - 1]->deliver(sn, id);
  return true;
}

UniformFIFOReliable::ProcessData::ProcessData(
    UniformFIFOReliable* fifo_urb, std::shared_ptr<spdlog::logger> file_logger)
    : fifo_urb_(fifo_urb), file_logger_(file_logger), next_(0) {}

void UniformFIFOReliable::ProcessData::deliver(int sn, int msg_id) {
  std::unique_lock<std::mutex> lock(mutex_);
  pending_messages_[sn] = msg_id;
  auto it = pending_messages_.find(next_);
  while (it != pending_messages_.end()) {
    const std::string* msg = fifo_urb_->identity_manager_.getValue(it->second);
    LOG("FIFO Reliable Delivered: ", util::stringToBinary(msg));
    file_logger_->info("d {} {}", unpackProcessId(*msg), unpackSn(*msg) + 1);
    std::cout << "Delivered message " << unpackSn(*msg) << std::endl;
    next_ += 1;
    pending_messages_.erase(it);
    it = pending_messages_.find(next_);
  }
}

}  // namespace broadcast
}  // namespace da
