#include <da/receiver/receiver.h>

#include <utility>

#include <da/util/logging.h>

namespace da {
namespace receiver {
namespace {

std::pair<int, int> getProcessIdAndMessage(const char* buffer) {
  int process_id = 0;
  for (int i = 1; i <= int(sizeof(int)); i++) {
    // Clear off any sign extension that might have happened.
    int byte = static_cast<int>(buffer[i - 1]) & 0xFF;
    byte <<= (8 * (sizeof(int) - i));
    process_id |= byte;
  }
  int message = 0;
  for (int i = 1; i <= int(sizeof(int)); i++) {
    // Clear off any sign extension that might have happened.
    int byte = static_cast<int>(buffer[sizeof(int) + i - 1]) & 0xFF;
    byte <<= (8 * (sizeof(int) - i));
    message |= byte;
  }
  return {process_id, message};
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
