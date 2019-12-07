#include <da/broadcast/fifo.h>

#include <memory>
#include <string>

#include <da/broadcast/uniform_reliable.h>
#include <da/da_proc.h>
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

// Assumes that the message has a valid minimum length.
inline int unpackSn(const std::string& msg) {
  return util::stringToInteger<int>(msg.data() + sizeof(uint16_t));
}

// Assumes that the message has a valid minimum length.
inline int unpackMessage(const std::string& msg) {
  return util::stringToInteger<int>(msg.data() + sizeof(uint16_t) +
                                    sizeof(int));
}

}  // namespace

const int fifo_min_length = urb_min_length + sizeof(uint16_t) + 2 * sizeof(int);

UniformFIFOReliable::UniformFIFOReliable(const process::Process* local_process,
                                         std::unique_ptr<UniformReliable> urb,
                                         int processes,
                                         spdlog::logger* file_logger)
    : local_process_(local_process),
      urb_(std::move(urb)),
      lsn_(0),
      file_logger_(file_logger),
      delivered_msgs_(0),
      broadcast_msgs_(0) {
  process_data_.reserve(processes);
  for (int i = 0; i < processes; i++) {
    process_data_.emplace_back(std::make_unique<ProcessData>(this));
  }
}

int UniformFIFOReliable::constructIdentity(const std::string* msg) {
  using namespace std::string_literals;
  std::string broadcast_msg = ""s;
  broadcast_msg += util::integerToString<uint16_t>(local_process_->getId());
  broadcast_msg += util::integerToString<int>(lsn_++);
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

bool UniformFIFOReliable::shouldBroadcast() {
  int num_threads = std::thread::hardware_concurrency();
  int num_delivered =
      process_data_[local_process_->getId()]->getDeliveredMessages();
  if (broadcast_msgs_ - num_delivered > 4375 * std::min(num_threads, 6)) {
    return false;
  }
  // FIXME: Correct the heuristic.
  // int num_undelivered = 0;
  // for (const auto& process_data : process_data_) {
  //   num_undelivered += process_data->getUndeliveredMessages();
  // }
  // if (num_undelivered > 2500 * std::min(num_threads, 6)) {
  //   return false;
  // }
  return true;
}

void UniformFIFOReliable::broadcast(const std::string* msg) {
  while (!shouldBroadcast()) {
    // Sleep for 1 milli second(s).
    util::nanosleep(1000000);
    if (kCanStop) {
      return;
    }
  }
  broadcast_msgs_ += 1;
  int id = constructIdentity(msg);
  file_logger_->info("b {}", da::util::stringToInteger<int>(*msg));
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
  if (process_id < 0 || process_id >= int(process_data_.size())) {
    LOG("Received an irrelevant process id: ", process_id + 1, ". Skipping.");
    return false;
  }
  process_data_[process_id]->deliver(sn, id);
  return true;
}

UniformFIFOReliable::ProcessData::ProcessData(UniformFIFOReliable* fifo_urb)
    : fifo_urb_(fifo_urb), next_(0), delivered_msgs_(0), max_sn_(0) {}

int UniformFIFOReliable::ProcessData::getDeliveredMessages() const {
  return delivered_msgs_;
}

int UniformFIFOReliable::ProcessData::getUndeliveredMessages() const {
  return max_sn_ - next_ - pending_messages_.size();
}

void UniformFIFOReliable::ProcessData::deliver(int sn, int msg_id) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (sn > max_sn_) {
    max_sn_ = sn;
  }
  pending_messages_[sn] = msg_id;
  auto it = pending_messages_.find(next_);
  while (it != pending_messages_.end()) {
    const std::string* msg = fifo_urb_->identity_manager_.getValue(it->second);
    LOG("FIFO Reliable Delivered: ", util::stringToBinary(msg));
    fifo_urb_->file_logger_->info("d {} {}", unpackProcessId(*msg) + 1,
                                  unpackMessage(*msg));
    fifo_urb_->delivered_msgs_ += 1;
    delivered_msgs_ += 1;
    next_ += 1;
    pending_messages_.erase(it);
    it = pending_messages_.find(next_);
  }
}

}  // namespace broadcast
}  // namespace da
