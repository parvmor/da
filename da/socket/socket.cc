#include <da/socket/socket.h>

namespace da {
namespace socket {

util::Status fillAddr(const std::string &address, unsigned short port,
                      sockaddr_in &addr) {
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  inet_pton(AF_INET, address.c_str(), &addr.sin_addr);
  return util::Status();
}

Socket::Socket(int type, int port) throw() {
  if ((sockDesc_ = ::socket(PF_INET, type, port)) < 0) {
    throw util::RuntimeStatusError(util::Status(
        util::StatusCode::kUnknown, "Socket creation failed (socket())"));
  }
}

Socket::Socket(int sockDesc) : sockDesc_(sockDesc) {}

Socket::~Socket() {
  close(sockDesc_);
  sockDesc_ = -1;
}

util::StatusOr<std::string> Socket::getLocalAddress() {
  sockaddr_in addr;
  unsigned int addr_len = sizeof(addr);
  if (getsockname(sockDesc_, (sockaddr *)&addr, (socklen_t *)&addr_len) < 0) {
    return util::Status(util::StatusCode::kUnknown,
                        "Fetch of local address failed (getsockname())");
  }
  return std::string(inet_ntoa(addr.sin_addr));
}

util::StatusOr<unsigned short> Socket::getLocalPort() {
  sockaddr_in addr;
  unsigned int addr_len = sizeof(addr);
  if (getsockname(sockDesc_, (sockaddr *)&addr, (socklen_t *)&addr_len) < 0) {
    return util::Status(util::StatusCode::kUnknown,
                        "Fetch of local port failed (getsockname())");
  }
  return ntohs(addr.sin_port);
}

util::Status Socket::setLocalPort(unsigned short localPort) {
  // Bind the socket to its port.
  sockaddr_in localAddr;
  memset(&localAddr, 0, sizeof(localAddr));
  localAddr.sin_family = AF_INET;
  localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  localAddr.sin_port = htons(localPort);
  if (bind(sockDesc_, (sockaddr *)&localAddr, sizeof(sockaddr_in)) < 0) {
    return util::Status(util::StatusCode::kUnknown,
                        "Set of local port failed (bind())");
  }
  return util::Status();
}

util::Status Socket::setLocalAddressAndPort(const std::string &localAddress,
                                            unsigned short localPort) {
  // Get the address of the requested host.
  sockaddr_in localAddr;
  const auto status = fillAddr(localAddress, localPort, localAddr);
  if (!status.ok()) {
    return status;
  }
  if (bind(sockDesc_, (sockaddr *)&localAddr, sizeof(sockaddr_in)) < 0) {
    return util::Status(util::StatusCode::kUnknown,
                        "Set of local address and port failed (bind())");
  }
  return util::Status();
}

unsigned short Socket::resolveService(const std::string &service,
                                      const std::string &protocol) {
  struct servent *serv;
  if ((serv = getservbyname(service.c_str(), protocol.c_str())) == NULL) {
    return atoi(service.c_str());
  }
  return ntohs(serv->s_port);
}

}  // namespace socket
}  // namespace da
