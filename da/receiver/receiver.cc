#include <da/receiver/receiver.h>

#include <functional>
#include <memory>

#include <da/message/message.h>
#include <da/util/logging.h>

namespace da {
namespace receiver {

void Receiver::operator()(broadcast::UniformReliable* urb) {
  while (alive_) {
    auto buffer = new char[link::max_length];
    const auto int_or = sock_->recv(buffer, link::max_length);
    if (!int_or.ok()) {
      LOG("Receiving from the socket failed. Status: ", int_or.status());
      continue;
    }
    if (int_or.value() < link::min_length) {
      LOG("Unable to receive a message of length atleast ", link::min_length,
          " from the socket. Received length: ", int_or.value());
      continue;
    }
    executor_->add(
        [&urb, buffer]() { urb->deliver(std::unique_ptr<char[]>(buffer)); });
  }
}

}  // namespace receiver
}  // namespace da
