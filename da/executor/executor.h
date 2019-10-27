#ifndef __INCLUDED_DA_EXECUTOR_EXECUTOR_H_
#define __INCLUDED_DA_EXECUTOR_EXECUTOR_H_

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
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

  // Adds the function with a delay of interval microseconds in execution.
  // If there is no other function waiting to be executed sooner then the given
  // function will be executed.
  template <typename Function, typename... Args>
  auto schedule(std::chrono::microseconds interval, Function&& f,
                Args&&... args) -> std::future<decltype(f(args...))>;

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

template <typename Function, typename... Args>
auto Executor::add(Function&& f, Args&&... args)
    -> std::future<decltype(f(args...))> {
  return schedule(std::chrono::microseconds::zero(), f, args...);
}

template <typename Function, typename... Args>
auto Executor::schedule(std::chrono::microseconds interval, Function&& f,
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

}  // namespace executor
}  // namespace da

#endif  // __INCLUDED_DA_EXECUTOR_EXECUTOR_H_
