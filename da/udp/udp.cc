#include "udp.h"

da::udp::Udp::Udp(const char *ip, int port, int domain, int type,
                  int protocol) {
  struct sockaddr_in serv_addr;

  // Create new socket descriptor
  sock = socket(domain, type, protocol);
  if (sock < 0) {
    perror("Socket creation failed.");
    throw errno;
  } else {
    perror("Socket successfully created.");
  }

  // setup struct for current address
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  inet_aton(ip, &serv_addr.sin_addr);

  // bind socket to the current address
  int binded =
      bind(sock, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
  if (binded < 0) {
    perror("Socket binding failed.");
    close(sock);
    throw errno;
  } else {
    perror("Socket successfully bound.");
  }
}

int da::udp::Udp::recv(long timeout_s, long timeout_ns, char *buffer,
                       struct sockaddr_in *servaddr, socklen_t *len) {
  // Define timeval for the timeout
  timeout_s += timeout_ns / 1000000;
  timeout_ns = timeout_ns % 1000000;
  struct timeval tm;
  tm.tv_usec = timeout_ns;
  tm.tv_sec = timeout_s;

  // Set timeout for recv operation
  if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tm, sizeof(tm)) < 0) {
    perror("Setting recv timeout failed");
  } else {
    perror("Setting recv timeout successful.");
  }

  // Free space for serveraddr
  memset(servaddr, 0, sizeof(sockaddr_in));
  memset(len, 0, sizeof(socklen_t));

  // Receive message
  int n = recvfrom(sock, buffer, BUFFERSIZE, MSG_WAITALL,
                   (struct sockaddr *)servaddr, len);
  if (n < 0) {
    perror("Error while trying to receive message.");
  }
  buffer[n] = '\0';
  return n;
}

int da::udp::Udp::send(const char *ip, int port, const char *msg) {

  // setup descriptor for target address
  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  inet_aton(ip, &serv_addr.sin_addr);

  // send message
  int n = sendto(sock, msg, strlen(msg), MSG_CONFIRM,
                 (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
  if (n < 0) {
    perror("Error while sending message");
  }
  return n;
}

da::udp::Udp::~Udp() { close(sock); }