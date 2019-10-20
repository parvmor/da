#include <cassert>
#include <memory>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <time.h>

#include <da/executor/executor.h>
#include <da/init/parser.h>
#include <da/link/perfect_link.h>
#include <da/receiver/receiver.h>
#include <da/socket/udp_socket.h>
#include <da/util/logging.h>
#include <da/util/statusor.h>

static int wait_for_start = 1;

static void start(int signum) { wait_for_start = 0; }

static void stop(int signum) {
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

int main(int argc, char** argv) {
  // Parse the membership file.
  const auto processes_or = da::init::parse(argc, argv);
  if (!processes_or.ok()) {
    LOG("Unable to parse the command line arguments. Status: ",
        processes_or.status());
    return 1;
  }
  // Find the current process class.
  da::process::Process* current_process = nullptr;
  const auto& processes = *processes_or;
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
  // Launch a thread that will be receiving packets.
  auto receiver =
      std::make_unique<da::receiver::Receiver>(executor.get(), sock.get());
  auto receiver_thread = std::thread([&receiver]() { (*receiver)(); });
  da::link::PerfectLink perfect_link(executor.get(), sock.get(),
                                     current_process, processes[0].get());
  perfect_link.sendMessage(123212);
  sleep(1);
  // Stop and wait for receiver thread to exit gracefully.
  receiver->stop();
  if (receiver_thread.joinable()) {
    receiver_thread.join();
  }
  return 0;
  // set signal handlers
  signal(SIGUSR2, start);
  signal(SIGTERM, stop);
  signal(SIGINT, stop);

  // parse arguments, including membership
  // initialize application
  // start listening for incoming UDP packets
  printf("Initializing.\n");

  // wait until start signal
  while (wait_for_start) {
    struct timespec sleep_time;
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = 1000;
    nanosleep(&sleep_time, NULL);
  }

  // broadcast messages
  printf("Broadcasting messages.\n");

  // wait until stopped
  while (1) {
    struct timespec sleep_time;
    sleep_time.tv_sec = 1;
    sleep_time.tv_nsec = 0;
    nanosleep(&sleep_time, NULL);
  }
}
