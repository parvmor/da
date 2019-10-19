#ifndef __INCLUDED_DA_EXECUTOR_EXECUTOR_H_
#define __INCLUDED_DA_EXECUTOR_EXECUTOR_H_

#include <atomic>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include <da/executor/thread_safe_queue.h>

namespace da {
namespace executor {

class Executor {
 public:
  Executor();

  Executor(int no_of_threads);

  ~Executor();

  // Delete the copy constructor.
  Executor(const Executor&) = delete;
  // Delete the move constructor.
  Executor(Executor&&) = delete;
  // Delete the copy assignment operator.
  Executor& operator=(const Executor&) = delete;
  // Delete the move assignment operator.
  Executor& operator=(Executor&&) = delete;

  bool isAlive() { return alive_; }

  void waitForCompletion();

  template <typename Function, typename... Args>
  auto add(Function&& f, Args&&... args) -> std::future<decltype(f(args...))>;

 private:
  class Worker;

  std::atomic<bool> alive_;
  std::vector<std::thread> workers_;
  ThreadSafeQueue<std::function<void()>> queue_;
  std::mutex mutex_;
  std::condition_variable cond_var_;
};

class Executor::Worker {
 public:
  Worker(int id, Executor* executor_);

  void operator()();

 private:
  const int id_;
  Executor* executor_;
};

Executor::Executor() : Executor(std::thread::hardware_concurrency()) {}

Executor::Executor(int no_of_threads)
    : alive_(true), workers_(std::vector<std::thread>(no_of_threads)) {
  for (int id = 0; id < int(workers_.size()); id++) {
    workers_[id] = std::thread(Worker(id, this));
  }
}

Executor::~Executor() {
  if (alive_) {
    waitForCompletion();
  }
}

void Executor::waitForCompletion() {
  {
    alive_ = false;
    std::unique_lock<std::mutex> lock(mutex_);
    cond_var_.notify_all();
  }
  for (int id = 0; id < int(workers_.size()); id++) {
    if (workers_[id].joinable()) {
      workers_[id].join();
    }
  }
}

template <typename Function, typename... Args>
auto Executor::add(Function&& f, Args&&... args)
    -> std::future<decltype(f(args...))> {
  // Bind the arguments to the function.
  std::function<decltype(f(args...))()> func =
      std::bind(std::forward<Function>(f), std::forward<Args>(args)...);
  // Wrap it in a packaged task so that we function definition is now void().
  auto task =
      std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
  // Correct the type and enqueue it to be executed.
  std::function<void()> ret_void_func = [task]() { (*task)(); };
  queue_.enqueue(ret_void_func);
  // Wake up a thread to execute the function (if it was waiting).
  {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_var_.notify_one();
  }
  // Return a future if there was a return type expected.
  return task->get_future();
}

Executor::Worker::Worker(int id, Executor* executor)
    : id_(id), executor_(executor) {}

void Executor::Worker::operator()() {
  while (executor_->alive_) {
    std::function<void()> f;
    {
      std::unique_lock<std::mutex> lock(executor_->mutex_);
      if (executor_->queue_.empty()) {
        executor_->cond_var_.wait(lock, [this] {
          return !executor_->alive_ || !executor_->queue_.empty();
        });
      }
      if (!executor_->queue_.dequeue(f)) {
        continue;
      }
    }
    f();
  }
}

}  // namespace executor
}  // namespace da

#endif  // __INCLUDED_DA_EXECUTOR_EXECUTOR_H_
