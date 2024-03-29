#ifndef __INCLUDED_DA_RECEIVER_RECEIVER_H_
#define __INCLUDED_DA_RECEIVER_RECEIVER_H_

#include <atomic>

#include <da/broadcast/fifo.h>
#include <da/broadcast/localized_causal.h>
#include <da/executor/executor.h>
#include <da/link/perfect_link.h>
#include <da/socket/udp_socket.h>

namespace da {
namespace receiver {

class Receiver {
 public:
  Receiver(executor::Executor* executor, socket::UDPSocket* sock)
      : alive_(true), executor_(executor), sock_(sock) {}

  ~Receiver() { stop(); }

  void stop() { alive_ = false; }

  bool isAlive() const { return alive_; }

  void operator()(broadcast::UniformFIFOReliable* fifo_urb);

  void operator()(broadcast::UniformLocalizedCausal* lc_urb);

 private:
  std::atomic<bool> alive_;
  executor::Executor* executor_;
  socket::UDPSocket* sock_;
};

}  // namespace receiver
}  // namespace da

#endif  // __INCLUDED_DA_RECEIVER_RECEIVER_H_
