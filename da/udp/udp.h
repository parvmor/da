#ifndef __INCLUDED_DA_UDP_UDP_H_
#define __INCLUDED_DA_UDP_UDP_H_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <string>

#define BUFFERSIZE 1024

namespace da {
namespace udp {
class Udp {
 private:
  int sock;

 public:
  // Initialize a udp socket.
  Udp(const char* ip, int port, int domain = AF_INET, int type = SOCK_DGRAM,
      int protocol = 0);
  ~Udp();

  // Receives a message. The buffer should have length BUFFERSIZE.
  int recv(long timeout_s, long timeout_ns, char* buffer,
           struct sockaddr_in* servaddr, socklen_t* len);
  // Sends the given message.
  int send(const char* ip, int port, const char* msg);
};
}  // namespace udp
}  // namespace da
#endif