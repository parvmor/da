#include <da/link/perfect_link.h>

#include <chrono>
#include <cstring>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>

#include <da/executor/executor.h>
#include <da/process/process.h>
#include <da/socket/udp_socket.h>
#include <da/util/logging.h>
#include <da/util/status.h>
#include <da/util/statusor.h>
#include <da/util/util.h>

namespace da {
namespace link {

const int max_length = 256 * sizeof(int);
const int min_length = 2 * sizeof(int) + sizeof(bool);

PerfectLink::PerfectLink(executor::Executor* executor, socket::UDPSocket* sock,
                         const process::Process* local_process,
                         const process::Process* foreign_process)
    : PerfectLink(executor, sock, local_process, foreign_process,
                  std::chrono::microseconds(100)) {}

PerfectLink::PerfectLink(executor::Executor* executor, socket::UDPSocket* sock,
                         const process::Process* local_process,
                         const process::Process* foreign_process,
                         std::chrono::microseconds interval)
    : executor_(executor),
      sock_(sock),
      local_process_(local_process),
      foreign_process_(foreign_process),
      interval_(interval) {}

PerfectLink::~PerfectLink() {
  std::unique_lock<std::shared_timed_mutex> lock(mutex_);
  undelivered_messages_.clear();
}

void PerfectLink::sendMessage(std::shared_ptr<std::string> message) {
  {
    std::unique_lock<std::shared_timed_mutex> lock(mutex_);
    undelivered_messages_.insert(*message);
  }
  sendMessageCallback(message);
}

void PerfectLink::sendMessageCallback(std::shared_ptr<std::string> message) {
  {
    std::shared_lock<std::shared_timed_mutex> lock(mutex_);
    if (undelivered_messages_.find(*message) == undelivered_messages_.end()) {
      return;
    }
  }
  // The message is non-ascii and hence, cannot be printed.
  LOG("Sending message '", util::stringToBinary(message.get()), "' of length ",
      message->size(), " to ", *foreign_process_);
  const auto status =
      sock_->sendTo(message->data(), message->size(),
                    foreign_process_->getIPAddr(), foreign_process_->getPort());
  if (!status.ok()) {
    LOG("Sending of message '", util::stringToBinary(message.get()),
        "' of length ", message->size(), " to ", *foreign_process_,
        " failed. Status: ", status);
  }
  executor_->schedule(
      interval_, std::bind(&PerfectLink::sendMessageCallback, this, message));
}

void PerfectLink::ackMessage(const std::string& msg_str) {
  const auto status =
      sock_->sendTo(msg_str.data(), msg_str.size(),
                    foreign_process_->getIPAddr(), foreign_process_->getPort());
  if (!status.ok()) {
    LOG("Sending of message '", util::stringToBinary(&msg_str), "' to ",
        *foreign_process_, " failed. Status: ", status);
  }
}

bool PerfectLink::recvMessage(const message::Message* message) {
  const auto msg_ack_str = message->toAckString();
  if (message->isAck()) {
    {
      std::unique_lock<std::shared_timed_mutex> lock(mutex_);
      if (undelivered_messages_.find(msg_ack_str) ==
          undelivered_messages_.end()) {
        LOG("Unable to find the message: ", *message, "in ",
            "undelivered_messages_");
        return false;
      }
      undelivered_messages_.erase(msg_ack_str);
    }
    return false;
  } else {
    ackMessage(msg_ack_str);
  }
  const auto msg_str = message->toString();
  std::unique_lock<std::shared_timed_mutex> lock(mutex_);
  if (delivered_messages_.find(msg_str) != delivered_messages_.end()) {
    // We have already received this message.
    return false;
  }
  delivered_messages_.insert(msg_str);
  return true;
}

}  // namespace link
}  // namespace da
