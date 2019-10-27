#ifndef __INCLUDED_DA_RECEIVER_RECEIVER_H_
#define __INCLUDED_DA_RECEIVER_RECEIVER_H_

#include <atomic>

#include <da/broadcast/uniform_reliable.h>
#include <da/executor/executor.h>
#include <da/link/perfect_link.h>
#include <da/socket/udp_socket.h>

namespace da {
namespace receiver {

class Receiver {
 public:
  Receiver(executor::Executor* executor, socket::UDPSocket* sock,
           std::vector<std::unique_ptr<da::link::PerfectLink>>* perfect_links)
      : alive_(true),
        executor_(executor),
        sock_(sock),
        perfect_links_(perfect_links) {}

  ~Receiver() { alive_ = false; }

  void stop() { alive_ = false; }

  bool isAlive() const { return alive_; }

  void operator()();

 private:
  std::atomic<bool> alive_;
  executor::Executor* executor_;
  socket::UDPSocket* sock_;
  std::vector<std::unique_ptr<da::link::PerfectLink>>* perfect_links_;
};

}  // namespace receiver
}  // namespace da

#endif  // __INCLUDED_DA_RECEIVER_RECEIVER_H_
