#include <da/link/perfect_link.h>

#include <chrono>
#include <cstring>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>

#include <da/broadcast/uniform_reliable.h>
#include <da/executor/executor.h>
#include <da/process/process.h>
#include <da/socket/udp_socket.h>
#include <da/util/logging.h>
#include <da/util/status.h>
#include <da/util/statusor.h>
#include <da/util/util.h>

namespace da {
namespace link {
namespace {

std::string constructMessage(int process_id, int ack, std::string message) {
  return util::integerToString(process_id) + util::integerToString(ack) +
         message;
}

}  // namespace

const int msg_length = 255 * sizeof(int);

PerfectLink::PerfectLink(executor::Executor* executor, socket::UDPSocket* sock,
                         const process::Process* local_process,
                         const process::Process* foreign_process)
    : PerfectLink(executor, sock, local_process, foreign_process,
                  std::chrono::microseconds(1000)) {}

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

void PerfectLink::setUrb(std::unique_ptr<broadcast::UniformReliable>* urb) {
  this->urb_ = urb;
}

void PerfectLink::sendMessage(std::string message) {
  {
    std::unique_lock<std::shared_timed_mutex> lock(mutex_);
    undelivered_messages_.insert(message);
  }
  sendMessageCallback(message);
}

std::tuple<int, int, const char*> getProcessIdAndMessage(const char* buffer) {
  return {util::stringToInteger(buffer),
          util::stringToInteger(buffer + sizeof(int)),
          buffer + 2 * sizeof(int)};
}

void PerfectLink::sendMessageCallback(std::string message) {
  {
    std::shared_lock<std::shared_timed_mutex> lock(mutex_);
    if (undelivered_messages_.find(message) == undelivered_messages_.end()) {
      return;
    }
  }
  const auto msg_str =
      constructMessage(local_process_->getId(), 0, message).data();

  // LOG("Sending message '", local_process_->getId(), " ", message,
  //    "' of length ", msg_length, " to ", *foreign_process_);
  const auto status =
      sock_->sendTo(msg_str, msg_length, foreign_process_->getIPAddr(),
                    foreign_process_->getPort());
  if (!status.ok()) {
    LOG("Sending of message '", local_process_->getId(), " ", message,
        "' of length ", msg_length, " to ", *foreign_process_,
        " failed. Status: ", status);
  }
  executor_->schedule(
      interval_, std::bind(&PerfectLink::sendMessageCallback, this, message));
}

void PerfectLink::sendAckMessage(std::string message) {
  const auto msg_str =
      constructMessage(local_process_->getId(), 1, message).data();
  // LOG("Sending message '", local_process_->getId(), " ", message,
  //    "' of length ", msg_length, " to ", *foreign_process_);
  const auto status =
      sock_->sendTo(msg_str, msg_length, foreign_process_->getIPAddr(),
                    foreign_process_->getPort());
  if (!status.ok()) {
    LOG("Sending of message '", local_process_->getId(), " ", message,
        "' of length ", msg_length, " to ", *foreign_process_,
        " failed. Status: ", status);
  }
}

bool PerfectLink::recvMessage(std::string message, int ack) {
  /*if (message < 1 || message > local_process_->getMessageCount()) {
    LOG("Received an unknown message: ", message, " from the process ",
        foreign_process_, " by process ", local_process_);
    return false;
  }*/
  if (ack == 1) {
    {
      std::shared_lock<std::shared_timed_mutex> lock(mutex_);
      undelivered_messages_.erase(message);
    }
    return false;
  } else if (ack == 0) {
    sendAckMessage(message);
  }
  // LOG("Received message '", message, "' from process ", foreign_process_,
  //    " by process ", local_process_);
  std::unique_lock<std::shared_timed_mutex> lock(mutex_);
  if (delivered_messages_.find(message) != delivered_messages_.end()) {
    // We have already received this message.
    return false;
  }
  delivered_messages_.insert(message);

  std::unique_ptr<broadcast::UniformReliable>* urb = urb_;
  int id = (*foreign_process_).getId();
  executor_->add([urb, id, message]() { (*urb)->deliver(id, message); });
  return true;
}

}  // namespace link
}  // namespace da
