#include <da/receiver/receiver.h>

#include <utility>

#include <da/util/logging.h>
#include <da/util/util.h>

namespace da {
namespace receiver {
namespace {

std::pair<int, int> getProcessIdAndMessage(const char* buffer) {
  return {util::stringToInteger(buffer),
          util::stringToInteger(buffer + sizeof(int))};
}

}  // namespace

void Receiver::operator()(broadcast::UniformReliable* urb) {
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
    int process_id, message;
    std::tie(process_id, message) = getProcessIdAndMessage(buffer);
    LOG("Received message: ", message, " from process with id ", process_id);
    executor_->add([&urb, process_id, message]() {
      urb->deliver(process_id, message);
    });
  }
}

}  // namespace executor
}  // namespace da
