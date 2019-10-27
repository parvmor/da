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
namespace {

// Assumes that message has a valid minimum length.
bool isAckMessage(const std::string& msg) {
  return util::stringToBool(msg.data() + sizeof(int));
}

// Assumes that message has a valid minimum length.
std::string constructInverseMessage(std::string msg, int process_id, bool ack) {
  std::string process_id_str = util::integerToString(process_id);
  for (int i = 0; i < int(sizeof(int)); i++) {
    msg[i] = process_id_str[i];
  }
  msg[sizeof(int)] = util::boolToString(ack)[0];
  return msg;
}

}  // namespace

const int max_length = 256 * sizeof(int);
const int min_length = sizeof(int) + sizeof(bool);

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

int PerfectLink::constructIdentity(const std::string* msg) {
  using namespace std::string_literals;
  std::string link_msg = ""s;
  link_msg += util::integerToString(local_process_->getId());
  link_msg += util::boolToString(false);
  link_msg += *msg;
  return identity_manager_.assignId(link_msg);
}

void PerfectLink::sendMessage(const std::string* msg) {
  int id = constructIdentity(msg);
  {
    std::unique_lock<std::shared_timed_mutex> lock(mutex_);
    undelivered_messages_.insert(id);
  }
  sendMessageCallback(id);
}

void PerfectLink::sendMessageCallback(int id) {
  {
    std::shared_lock<std::shared_timed_mutex> lock(mutex_);
    if (undelivered_messages_.find(id) == undelivered_messages_.end()) {
      return;
    }
  }
  const std::string* msg = identity_manager_.getValue(id);
  // The message is non-ascii and hence, cannot be printed.
  LOG("Sending message '", util::stringToBinary(msg), "' to ",
      *foreign_process_);
  const auto status =
      sock_->sendTo(msg->data(), msg->size(), foreign_process_->getIPAddr(),
                    foreign_process_->getPort());
  if (!status.ok()) {
    LOG("Sending of message '", util::stringToBinary(msg), "' to ",
        *foreign_process_, " failed. Status: ", status);
  }
  executor_->schedule(interval_,
                      std::bind(&PerfectLink::sendMessageCallback, this, id));
}

void PerfectLink::ackMessage(const std::string& msg) {
  const std::string ack_msg =
      constructInverseMessage(msg, local_process_->getId(), true);
  const auto status =
      sock_->sendTo(ack_msg.data(), ack_msg.size(),
                    foreign_process_->getIPAddr(), foreign_process_->getPort());
  if (!status.ok()) {
    LOG("Sending of message '", util::stringToBinary(&ack_msg), "' to ",
        *foreign_process_, " failed. Status: ", status);
  }
}

bool PerfectLink::recvMessage(const std::string& msg) {
  if (isAckMessage(msg)) {
    LOG("Received an ACK message: ", util::stringToBinary(&msg));
    // The inverse message must already be there in the identity manager.
    int id = identity_manager_.getId(
        constructInverseMessage(msg, local_process_->getId(), false));
    if (id == -1) {
      LOG("Found a -1....");
      return false;
    }
    {
      std::unique_lock<std::shared_timed_mutex> lock(mutex_);
      undelivered_messages_.erase(id);
    }
    return false;
  }
  ackMessage(msg);
  // This message might be a new one.
  int id = identity_manager_.assignId(msg);
  std::unique_lock<std::shared_timed_mutex> lock(mutex_);
  if (delivered_messages_.find(id) != delivered_messages_.end()) {
    // We have already received this message.
    LOG("Found a already delievered message: ", id);
    return false;
  }
  delivered_messages_.insert(id);
  return true;
}

}  // namespace link
}  // namespace da
