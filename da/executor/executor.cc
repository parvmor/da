#include <da/executor/executor.h>
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
      // If the task is scheduled to be executed in the future. Enque it, sleep
      // for 10ms and then continue.
      if (task.getTime() > std::chrono::high_resolution_clock::now()) {
        executor_->queue_.enqueue(task);
        struct timespec sleep_time;
        sleep_time.tv_sec = 0;
        sleep_time.tv_nsec = 10000000;
        nanosleep(&sleep_time, NULL);
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
