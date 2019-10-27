#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <atomic>
#include <cassert>
#include <memory>
#include <thread>

#include <da/broadcast/uniform_reliable.h>
#include <da/executor/executor.h>
#include <da/init/parser.h>
#include <da/link/perfect_link.h>
#include <da/receiver/receiver.h>
#include <da/socket/udp_socket.h>
#include <da/util/logging.h>
#include <da/util/statusor.h>

namespace {

std::atomic<bool> can_start{false};

void registerSignalHandlers() {
  // Register a function to toggle the can_start when SIGUSR2 is received.
  // NOTE: Lambda and function pointers have different types and hence, a
  // positive lambda has been used.
  signal(
      SIGUSR2, +[](int signal) { can_start = true; });
  // TODO(parvmor): Fix the following signal handlers.
  signal(
      SIGTERM, +[](int signal) { can_start = false; });
  signal(
      SIGINT, +[](int signal) { can_start = false; });
}

void stop(int signum) {
  // reset signal handlers to default
  signal(SIGTERM, SIG_DFL);
  signal(SIGINT, SIG_DFL);

  // immediately stop network packet processing
  printf("Immediately stopping network packet processing.\n");

  // write/flush output file if necessary
  printf("Writing output.\n");

  // exit directly from signal handler
  exit(0);
}

}  // namespace

int main(int argc, char** argv) {
  LOG("Initializing...");
  // Register different signal handlers.
  registerSignalHandlers();
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
  // Create executor and a UDP socket for current process.
  auto executor = std::make_unique<da::executor::Executor>();
  auto sock = std::make_unique<da::socket::UDPSocket>(
      current_process->getIPAddr(), current_process->getPort());
  // Create a list of perfect links to all the processes.
  std::vector<std::unique_ptr<da::link::PerfectLink>> perfect_links;
  perfect_links.reserve(processes.size());
  for (const auto& process : processes) {
    perfect_links.emplace_back(std::make_unique<da::link::PerfectLink>(
        executor.get(), sock.get(), current_process, process.get()));
  }

  // Create a uniform reliable broadcast object.
  auto urb = std::make_unique<da::broadcast::UniformReliable>(current_process,
                                                              &perfect_links);

  for (long unsigned int i = 0; i < perfect_links.size(); i++)
    perfect_links[i]->setUrb(&urb);

  // Launch a thread that will be receiving packets.
  auto receiver = std::make_unique<da::receiver::Receiver>(
      executor.get(), sock.get(), &perfect_links);
  auto receiver_thread = std::thread([&receiver]() { (*receiver)(); });
  // Loop until SIGUSR2 hasn't received.
  while (!can_start) {
    struct timespec sleep_time;
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = 1000;
    nanosleep(&sleep_time, NULL);
  }
  // Start to broadcast messages.
  LOG("Broadcasting messages...");
  for (int id = 1; id <= current_process->getMessageCount(); id++) {
    urb->broadcast(id);
  }
  while (can_start) {
    struct timespec sleep_time;
    sleep_time.tv_sec = 2;
    sleep_time.tv_nsec = 0;
    nanosleep(&sleep_time, NULL);
  }
  // Stop and wait for receiver thread to exit gracefully.
  receiver->stop();
  // TODO(parvmor): Exit gracefully.
  receiver_thread.~thread();
  return 0;
}
