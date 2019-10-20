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

namespace da {
namespace link {
namespace {

std::string constructMessage(int process_id, int message) {
  std::string msg;
  for (int i = 1; i <= int(sizeof(int)); i++) {
    msg += static_cast<char>((process_id >> (8 * (sizeof(int) - i))) & 0xFF);
  }
  for (int i = 1; i <= int(sizeof(int)); i++) {
    msg += static_cast<char>((message >> (8 * (sizeof(int) - i))) & 0xFF);
  }
  return msg;
}

}  // namespace

const int msg_length = 2 * sizeof(int);

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

void PerfectLink::sendMessage(int message) {
  {
    std::unique_lock<std::shared_timed_mutex> lock(mutex_);
    undelivered_messages_.insert(message);
  }
  sendMessageCallback(message);
}

void PerfectLink::sendMessageCallback(int message) {
  {
    std::shared_lock<std::shared_timed_mutex> lock(mutex_);
    if (undelivered_messages_.find(message) == undelivered_messages_.end()) {
      return;
    }
  }
  const auto msg_str =
      constructMessage(local_process_->getId(), message).data();
  LOG("Sending message '", local_process_->getId(), " ", message,
      "' of length ", msg_length, " to ", *foreign_process_);
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

void PerfectLink::recvMessage(int message) {
  std::unique_lock<std::shared_timed_mutex> lock(mutex_);
  if (delivered_messages_.find(message) != delivered_messages_.end()) {
    // We have already received this message.
    return;
  }
  delivered_messages_.insert(message);
  // TODO(parvmor): Call the callback that is registered with this perfect link.
  LOG("Received message '", message, "' from process ", foreign_process_,
      " by process ", local_process_);
}

}  // namespace link
}  // namespace da
