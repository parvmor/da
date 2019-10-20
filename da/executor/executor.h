#ifndef __INCLUDED_DA_EXECUTOR_EXECUTOR_H_
#define __INCLUDED_DA_EXECUTOR_EXECUTOR_H_

#include <atomic>
#include <chrono>
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

  bool isAlive() const { return alive_; }

  void waitForCompletion();

  // Adds the function to be executed as soon as possible.
  template <typename Function, typename... Args>
  auto add(Function&& f, Args&&... args) -> std::future<decltype(f(args...))>;

  // Adds the function with a delay of interval seconds in execution.
  // If there is no other function waiting to be executed sooner then the given
  // function will be executed.
  template <typename Function, typename... Args>
  auto schedule(std::chrono::seconds interval, Function&& f, Args&&... args)
      -> std::future<decltype(f(args...))>;

 private:
  class Task;
  class Worker;

  std::atomic<bool> alive_;
  std::vector<std::thread> workers_;
  ThreadSafeQueue<Task> queue_;
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

class Executor::Task {
 public:
  Task();

  Task(const std::function<void()>& f,
       std::chrono::time_point<std::chrono::high_resolution_clock> time);

  bool operator<(const Task& task) const;

  bool operator>(const Task& task) const;

  void operator()();

  std::chrono::time_point<std::chrono::high_resolution_clock> getTime() const;

 private:
  std::function<void()> f_;
  std::chrono::time_point<std::chrono::high_resolution_clock> time_;
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
  return schedule(std::chrono::seconds::zero(), f, args...);
}

template <typename Function, typename... Args>
auto Executor::schedule(std::chrono::seconds interval, Function&& f,
                        Args&&... args) -> std::future<decltype(f(args...))> {
  const auto time = std::chrono::high_resolution_clock::now() + interval;
  // Bind the arguments to the function.
  std::function<decltype(f(args...))()> func =
      std::bind(std::forward<Function>(f), std::forward<Args>(args)...);
  // Wrap it in a packaged task so that we function definition is now void().
  auto task =
      std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
  // Correct the type and enqueue it to be executed.
  std::function<void()> ret_void_func = [task]() { (*task)(); };
  queue_.enqueue(Task(ret_void_func, time));
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
  while (executor_->alive_ || !executor_->queue_.empty()) {
    Task task;
    {
      std::unique_lock<std::mutex> lock(executor_->mutex_);
      if (executor_->queue_.empty()) {
        executor_->cond_var_.wait(lock, [this] {
          return !executor_->alive_ || !executor_->queue_.empty();
        });
      }
      if (!executor_->queue_.dequeue(task)) {
        continue;
      }
    }
    task();
  }
}

Executor::Task::Task()
    : f_(nullptr),
      time_(std::chrono::time_point<std::chrono::high_resolution_clock>()) {}

Executor::Task::Task(
    const std::function<void()>& f,
    std::chrono::time_point<std::chrono::high_resolution_clock> time)
    : f_(f), time_(time) {}

bool Executor::Task::operator<(const Executor::Task& task) const {
  return this->time_ < task.getTime();
}

bool Executor::Task::operator>(const Executor::Task& task) const {
  return this->time_ > task.getTime();
}

std::chrono::time_point<std::chrono::high_resolution_clock>
Executor::Task::getTime() const {
  return time_;
}

void Executor::Task::operator()() {
  if (f_ == nullptr) {
    return;
  }
  f_();
}

}  // namespace executor
}  // namespace da

#endif  // __INCLUDED_DA_EXECUTOR_EXECUTOR_H_
