#include <da/receiver/receiver.h>

#include <memory>
#include <string>

#include <da/util/logging.h>

namespace da {
namespace receiver {

void Receiver::operator()(broadcast::UniformFIFOReliable* fifo_urb) {
  while (isAlive()) {
    auto buffer = std::make_unique<char[]>(broadcast::fifo_min_length);
    const auto int_or = sock_->recv(buffer.get(), broadcast::fifo_min_length);
    if (!int_or.ok()) {
      LOG("Receiving from the socket failed. Status: ", int_or.status());
      continue;
    }
    if (int_or.value() < broadcast::fifo_min_length) {
      LOG("Unable to receive a message of length atleast ",
          broadcast::fifo_min_length,
          " from the socket. Received length: ", int_or.value());
      continue;
    }
    std::string msg(buffer.get(), int_or.value());
    if (!isAlive()) {
      break;
    }
    executor_->add([&fifo_urb, msg = std::move(msg)]() {
      fifo_urb->deliver(std::move(msg));
    });
  }
}

}  // namespace receiver
}  // namespace da
