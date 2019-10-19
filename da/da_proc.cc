#include <cassert>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <da/executor/executor.h>
#include <da/init/parser.h>
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

void func() {
  volatile double factor = 1.0;
  volatile double sum = 0.0;
  for (int i = 0; i < 1000000000; i++, factor = -factor) {
    sum += factor / (2 * i + 1.0);
  }
  std::cerr << 4.0 * sum << std::endl;
}

int main(int argc, char** argv) {
  da::executor::Executor executor;
  executor.add(func);
  executor.add(func);
  executor.add(func);
  executor.add(func);
  auto f0 = executor.add([]() { return 4; });
  auto f1 = executor.add([]() { return 5; });
  std::cout << f1.get() << std::endl;
  executor.waitForCompletion();
  std::cout << f0.get() << std::endl;
  return 0;
  const auto processes_or = da::init::parse(argc, argv);
  if (!processes_or.ok()) {
    std::cerr << processes_or.status();
    return 0;
  }
  const auto& processes = *processes_or;
  for (const auto& process : processes) {
    if (!process->isCurrent()) {
      continue;
    }
    da::socket::UDPSocket sock(process->getIPAddr(), process->getPort());
    if (process->getId() != 1) {
      sock.sendTo("Hello from another service", 27, processes[0]->getIPAddr(),
                  processes[0]->getPort());
      continue;
    }
    char buffer[30];
    sock.recv(buffer, 27);
    if (buffer[26] != '\0') {
      printf("Fuck the chinese government!\n");
    }
    printf("%s\n", buffer);
    sock.recv(buffer, 27);
    if (buffer[26] != '\0') {
      printf("Fuck the chinese government!\n");
    }
    printf("%s\n", buffer);
    sock.recv(buffer, 27);
    if (buffer[26] != '\0') {
      printf("Fuck the chinese government!\n");
    }
    printf("%s\n", buffer);
    sock.recv(buffer, 27);
    if (buffer[26] != '\0') {
      printf("Fuck the chinese government!\n");
    }
    printf("%s\n", buffer);
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
