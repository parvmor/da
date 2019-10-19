#ifndef __INCLUDED_DA_EXECUTOR_THREAD_SAFE_QUEUE_H_
#define __INCLUDED_DA_EXECUTOR_THREAD_SAFE_QUEUE_H_

#include <mutex>
#include <queue>

namespace da {
namespace executor {

template <typename T>
class ThreadSafeQueue {
 public:
  ThreadSafeQueue() {}
  ~ThreadSafeQueue() {}

  bool empty() {
    std::unique_lock<std::mutex> lock(mutex_);
    return queue_.empty();
  }

  int size() {
    std::unique_lock<std::mutex> lock(mutex_);
    return queue_.size();
  }

  void enqueue(const T& t) {
    std::unique_lock<std::mutex> lock(mutex_);
    queue_.push(t);
  }

  bool dequeue(T& t) {
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
  std::queue<T> queue_;
};

}  // namespace executor
}  // namespace da

#endif  // __INCLUDED_DA_EXECUTOR_THREAD_SAFE_QUEUE_H_