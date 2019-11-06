#ifndef __INCLUDED_DA_EXECUTOR_THREAD_SAFE_QUEUE_H_
#define __INCLUDED_DA_EXECUTOR_THREAD_SAFE_QUEUE_H_

#include <atomic>
#include <mutex>
#include <queue>
#include <vector>

namespace da {
namespace executor {

template <typename T>
class ThreadSafeQueue {
 public:
  ThreadSafeQueue() : alive_(true) {}
  ~ThreadSafeQueue() {}

  bool empty() {
    if (!alive_) {
      return true;
    }
    std::unique_lock<std::mutex> lock(mutex_);
    return queue_.empty();
  }

  void stop() {
    alive_ = false;
    std::unique_lock<std::mutex> lock(mutex_);
    std::queue<T> empty_queue = std::queue<T>();
    swap(empty_queue, queue_);
  }

  int size() {
    std::unique_lock<std::mutex> lock(mutex_);
    return queue_.size();
  }

  void enqueue(const T& t) {
    if (!alive_) {
      return;
    }
    std::unique_lock<std::mutex> lock(mutex_);
    queue_.push(t);
  }

  bool dequeue(T& t) {
    if (!alive_) {
      return false;
    }
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.empty()) {
      return false;
    }
    t = std::move(queue_.front());
    queue_.pop();
    return true;
  }

 private:
  std::mutex mutex_;
  std::atomic<bool> alive_;
  std::queue<T> queue_;
};

}  // namespace executor
}  // namespace da

#endif  // __INCLUDED_DA_EXECUTOR_THREAD_SAFE_QUEUE_H_
