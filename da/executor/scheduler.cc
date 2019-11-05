#include <da/executor/scheduler.h>
#include <da/util/util.h>

namespace da {
namespace executor {

Scheduler::Scheduler() : Scheduler(std::thread::hardware_concurrency()) {}

Scheduler::Scheduler(int no_of_threads)
    : alive_(true), workers_(std::vector<std::thread>(no_of_threads)) {
  for (int id = 0; id < int(workers_.size()); id++) {
    workers_[id] = std::thread(Worker(id, this));
  }
}

Scheduler::~Scheduler() {
  if (isAlive()) {
    waitForCompletion();
  }
}

void Scheduler::waitForCompletion() {
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

Scheduler::Worker::Worker(int id, Scheduler* scheduler)
    : id_(id), scheduler_(scheduler) {}

void Scheduler::Worker::operator()() {
  while (scheduler_->isAlive()) {
    Task task;
    {
      std::unique_lock<std::mutex> lock(scheduler_->mutex_);
      if (scheduler_->queue_.empty()) {
        scheduler_->cond_var_.wait(lock, [this] {
          return !scheduler_->isAlive() || !scheduler_->queue_.empty();
        });
      }
      if (!scheduler_->isAlive()) {
        return;
      }
      if (!scheduler_->queue_.dequeue(task)) {
        continue;
      }
    }
    // If task is to be executed in the future enqueue it, sleep for 10
    // micro-second and continue.
    // There is no need to wake up another thread since, this thread will
    // execute the task in the next loop.
    if (task.getTime() > std::chrono::high_resolution_clock::now()) {
      scheduler_->queue_.enqueue(task);
      util::nanosleep(10000);
      continue;
    }
    task();
  }
}

Scheduler::Task::Task()
    : f_(nullptr),
      time_(std::chrono::time_point<std::chrono::high_resolution_clock>()) {}

Scheduler::Task::Task(
    const std::function<void()>& f,
    std::chrono::time_point<std::chrono::high_resolution_clock> time)
    : f_(f), time_(time) {}

bool Scheduler::Task::operator<(const Scheduler::Task& task) const {
  return this->time_ < task.getTime();
}

bool Scheduler::Task::operator>(const Scheduler::Task& task) const {
  return this->time_ > task.getTime();
}

std::chrono::time_point<std::chrono::high_resolution_clock>
Scheduler::Task::getTime() const {
  return time_;
}

void Scheduler::Task::operator()() {
  if (f_ == nullptr) {
    return;
  }
  f_();
}
}  // namespace executor
}  // namespace da
