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
  std::ostringstream msg_stream;
  msg_stream << process_id << ' ' << message;
  return msg_stream.str();
}

}  // namespace

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
      constructMessage(local_process_->getId(), message).c_str();
  // Include the null character in the length.
  const auto msg_len = strlen(msg_str) + 1;
  LOG("Sending message '", std::string(msg_str), "' of length ", msg_len,
      " to ", *foreign_process_);
  const auto status =
      sock_->sendTo(msg_str, msg_len, foreign_process_->getIPAddr(),
                    foreign_process_->getPort());
  if (!status.ok()) {
    LOG("Sending of message '", std::string(msg_str), "' of length ", msg_len,
        " to ", *foreign_process_, " failed. Status: ", status);
  }
  executor_->schedule(
      interval_, std::bind(&PerfectLink::sendMessageCallback, this, message));
}

}  // namespace link
}  // namespace da
