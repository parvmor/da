#include <da/receiver/receiver.h>

#include <memory>
#include <string>

#include <da/util/logging.h>

namespace da {
namespace receiver {

void Receiver::operator()(broadcast::UniformReliable* urb) {
  while (alive_) {
    auto buffer = std::make_unique<char[]>(link::max_length);
    const auto int_or = sock_->recv(buffer.get(), link::max_length);
    if (!int_or.ok()) {
      LOG("Receiving from the socket failed. Status: ", int_or.status());
      continue;
    }
    if (int_or.value() < broadcast::urb_min_length) {
      LOG("Unable to receive a message of length atleast ", link::min_length,
          " from the socket. Received length: ", int_or.value());
      continue;
    }
    std::string msg(buffer.get(), int_or.value());
    executor_->add(
        [&urb, msg = std::move(msg)]() { urb->deliver(std::move(msg)); });
  }
}

}  // namespace receiver
}  // namespace da
