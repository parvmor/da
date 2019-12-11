#include <da/broadcast/localized_causal.h>
#include <da/broadcast/uniform_reliable.h>
#include <da/da_proc.h>
#include <da/executor/executor.h>
#include <da/executor/scheduler.h>
#include <da/init/parser.h>
#include <da/link/perfect_link.h>
#include <da/receiver/receiver.h>
#include <da/socket/udp_socket.h>
#include <da/util/logging.h>
#include <da/util/statusor.h>
#include <da/util/util.h>
#include <signal.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <atomic>
#include <cassert>
#include <functional>
#include <memory>
#include <thread>

namespace da {

std::atomic<bool> kCanStop{false};

}  // namespace da

namespace {

std::atomic<bool> can_start{false};
std::shared_ptr<spdlog::logger> file_logger;
std::unique_ptr<da::executor::Executor> executor;
std::unique_ptr<da::executor::Scheduler> scheduler;
std::unique_ptr<da::receiver::Receiver> receiver;
std::unique_ptr<std::thread> receiver_thread;
std::unique_ptr<da::socket::UDPSocket> sock;

void registerUsrHandlers() {
  // Register a function to toggle the can_start when SIGUSR{1,2} is received.
  // NOTE: Lambda and function pointers have different types and hence, a
  // positive lambda has been used.
  signal(
      SIGUSR1, +[](int signum) { can_start = true; });
  signal(
      SIGUSR2, +[](int signum) { can_start = true; });
}

void exitHandler(int signum) {
  // Break free from infinite loops.
  da::kCanStop = true;
}

void exitThreads() {
  // Reset the handler to default one.
  signal(SIGTERM, SIG_DFL);
  signal(SIGINT, SIG_DFL);
  // Stop and wait for any intermediate tasks in executor to be completed.
  if (executor != nullptr) {
    LOG("Stopping the executor.");
    executor->waitForCompletion();
  }
  // Stop and wait for any intermediate tasks in scheduler to be completed.
  if (scheduler != nullptr) {
    LOG("Stopping the scheduler.");
    scheduler->waitForCompletion();
  }
  // Stop the receiver.
  if (receiver != nullptr) {
    LOG("Stopping the receiver.");
    receiver->stop();
  }
  // Stop the receiver thread.
  if (receiver_thread != nullptr && receiver_thread->joinable()) {
    LOG("Stopping the receiver thread.");
    receiver_thread->join();
  }
  // Disconnect the socket.
  if (sock != nullptr) {
    LOG("Disconnecting the socket.");
    const auto status = sock->disconnect();
    if (!status.ok()) {
      LOG("Failed to disconnect. Status: ", status);
    }
  }
  // Flush the output to the files.
  if (file_logger != nullptr) {
    LOG("Flushing the output.");
    file_logger->flush();
  }
}

void registerTermAndIntHandlers() {
  signal(SIGTERM, exitHandler);
  signal(SIGINT, exitHandler);
}

}  // namespace

int main(int argc, char** argv) {
  LOG("Initializing...");
  // Register the SIGUSR{1,2} signal handler.
  registerUsrHandlers();
  // Register the termination signal handlers.
  registerTermAndIntHandlers();
  // Parse the membership file.
  auto processes_or = da::init::parse(argc, argv);
  if (!processes_or.ok()) {
    LOG("Unable to parse the command line arguments. Status: ",
        processes_or.status());
    return 1;
  }
  // Find the current process class.
  da::process::Process* current_process = nullptr;
  auto& processes = *processes_or;
  for (const auto& process : processes) {
    da::broadcast::lcb_max_length =
        std::max(da::broadcast::lcb_max_length,
                 int(da::broadcast::urb_min_length + sizeof(uint16_t) +
                     (1 + process->getDependencies().size()) * sizeof(int)));
    if (!process->isCurrent()) {
      continue;
    }
    current_process = process.get();
  }
  if (current_process == nullptr) {
    LOG(da::util::Status(da::util::StatusCode::kNotFound,
                         "Could not find current process."));
    return 1;
  }
  // Create an in memory logger that will eventually flush to file.
  file_logger = spdlog::basic_logger_mt(
      "basic_logger",
      "da_proc_" + std::to_string(current_process->getId() + 1) + ".out", true);
  file_logger->set_pattern("%v");
  // Create executors and a UDP socket for current process.
  int num_threads = std::thread::hardware_concurrency() * 5;
  executor = std::make_unique<da::executor::Executor>(
      (num_threads + processes.size() - 1) / (processes.size()));
  scheduler = std::make_unique<da::executor::Scheduler>(1);
  // Create a socket with receive timeout of 1000 micro-seconds.
  struct timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  sock = std::make_unique<da::socket::UDPSocket>(
      current_process->getIPAddr(), current_process->getPort(), tv);
  // Create a list of perfect links to all the processes.
  std::vector<std::unique_ptr<da::link::PerfectLink>> perfect_links;
  perfect_links.reserve(processes.size());
  for (const auto& process : processes) {
    perfect_links.emplace_back(std::make_unique<da::link::PerfectLink>(
        scheduler.get(), sock.get(), current_process, process.get()));
  }
  // Create a uniform reliable broadcast object.
  auto urb = std::make_unique<da::broadcast::UniformReliable>(
      current_process, std::move(perfect_links));
  // Create a uniform localized causal reliable broadcast object.
  auto lc_urb = std::make_unique<da::broadcast::UniformLocalizedCausal>(
      current_process, std::move(urb), std::move(processes), file_logger.get());
  // Launch a thread that will be receiving packets.
  receiver =
      std::make_unique<da::receiver::Receiver>(executor.get(), sock.get());
  receiver_thread =
      std::make_unique<std::thread>([&lc_urb]() { (*receiver)(lc_urb.get()); });
  // Loop until SIGUSR2 hasn't received or an exit is called.
  while (!can_start && !da::kCanStop) {
    struct timespec sleep_time;
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = 1000;
    nanosleep(&sleep_time, NULL);
  }
  // Start to broadcast messages.
  LOG("Broadcasting messages...");
  for (int id = 1; id <= current_process->getMessageCount(); id++) {
    if (da::kCanStop) {
      break;
    }
    const std::string msg = da::util::integerToString<int>(id);
    lc_urb->broadcast(&msg);
  }
  // Loop infinitely. Signal handler will set `kCanStop` to false.
  while (!da::kCanStop) {
    // Sleep for 100 milli-seconds.
    struct timespec sleep_time;
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = 100000000;
    nanosleep(&sleep_time, NULL);
  }
  exitThreads();
  // Exit the program.
  std::cerr << "Exiting the program..." << std::endl;
  return 0;
}
