#include <da/receiver/receiver.h>

#include <utility>

#include <da/util/logging.h>
#include <da/util/util.h>

namespace da {
namespace receiver {
namespace {

std::tuple<int, int, std::string> getProcessIdAndMessage(const char* buffer) {
  return {util::stringToInteger(buffer),
          util::stringToInteger(buffer + sizeof(int)),
          std::string(buffer + 2 * sizeof(int))};
}

}  // namespace

void Receiver::operator()() {
  while (alive_) {
    char buffer[link::msg_length];
    const auto int_or = sock_->recv(buffer, link::msg_length);
    if (!int_or.ok()) {
      LOG("Receiving from the socket failed. Status: ", int_or.status());
      continue;
    }
    if (int_or.value() != link::msg_length) {
      LOG("Unable to receive a message of length ", link::msg_length,
          " from the socket. Received length: ", int_or.value());
      continue;
    }
    int process_id, ack;
    std::string message;
    std::tie(process_id, ack, message) = getProcessIdAndMessage(buffer);
    // LOG("RECEIVING FROM ", process_id, " MESSAGE ", message, " ACK ", ack);
    LOG("Received message: ", message, " from process with id: ", process_id);
    if (process_id < 1 || process_id > int((*perfect_links_).size())) {
      LOG("Received message: ", message,
          " from process with unknown id: ", process_id);
    } else {
      std::unique_ptr<da::link::PerfectLink>* link =
          &(*perfect_links_)[process_id - 1];
      executor_->add(
          [link, message, ack]() { (*link)->recvMessage(message, ack); });
    }
  }
}

}  // namespace receiver
}  // namespace da
