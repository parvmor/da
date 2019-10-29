#include <da/executor/executor.h>

#include <da/util/util.h>

namespace da {
namespace executor {

Executor::Executor() : Executor(std::thread::hardware_concurrency()) {}

Executor::Executor(int no_of_threads)
    : alive_(true), workers_(std::vector<std::thread>(no_of_threads)) {
  for (int id = 0; id < int(workers_.size()); id++) {
    workers_[id] = std::thread(Worker(id, this));
  }
}

Executor::~Executor() {
  if (isAlive()) {
    waitForCompletion();
  }
}

void Executor::waitForCompletion() {
  alive_ = false;
  queue_.stop();
  {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_var_.notify_all();
  }
  for (int id = 0; id < int(workers_.size()); id++) {
    if (workers_[id].joinable()) {
      workers_[id].join();
    }
  }
}

Executor::Worker::Worker(int id, Executor* executor)
    : id_(id), executor_(executor) {}

void Executor::Worker::operator()() {
  while (executor_->isAlive()) {
    std::function<void()> f;
    {
      std::unique_lock<std::mutex> lock(executor_->mutex_);
      if (executor_->queue_.empty()) {
        executor_->cond_var_.wait(lock, [this] {
          return !executor_->isAlive() || !executor_->queue_.empty();
        });
      }
      if (!executor_->isAlive()) {
        return;
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
