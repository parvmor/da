#include <da/socket/communicating_socket.h>
#include <da/socket/socket.h>

namespace da {
namespace socket {

CommunicatingSocket::CommunicatingSocket(int type, int protocol) throw()
    : Socket(type, protocol) {}

CommunicatingSocket::CommunicatingSocket(int sockDesc) : Socket(sockDesc) {}

util::Status CommunicatingSocket::connect(const std::string &foreignAddress,
                                          unsigned short foreignPort) {
  // Get the address of the requested host.
  sockaddr_in destAddr;
  const auto status = fillAddr(foreignAddress, foreignPort, destAddr);
  if (!status.ok()) {
    return status;
  }
  // Try to connect to the given port.
  if (::connect(sockDesc_, (sockaddr *)&destAddr, sizeof(destAddr)) < 0) {
    return util::Status(util::StatusCode::kUnknown,
                        "Connect failed (connect())");
  }
  return util::Status();
}

util::Status CommunicatingSocket::send(const void *buffer, int bufferLen) {
  if (::send(sockDesc_, (void *)buffer, bufferLen, 0) < 0) {
    return util::Status(util::StatusCode::kUnknown, "Send failed (send())");
  }
  return util::Status();
}

util::StatusOr<int> CommunicatingSocket::recv(void *buffer, int bufferLen) {
  int rtn;
  if ((rtn = ::recv(sockDesc_, (void *)buffer, bufferLen, 0)) < 0) {
    return util::Status(util::StatusCode::kUnknown, "Received failed (recv())");
  }
  return rtn;
}

util::StatusOr<std::string> CommunicatingSocket::getForeignAddress() {
  sockaddr_in addr;
  unsigned int addr_len = sizeof(addr);
  if (getpeername(sockDesc_, (sockaddr *)&addr, (socklen_t *)&addr_len) < 0) {
    return util::Status(util::StatusCode::kUnknown,
                        "Fetch of foreign address failed (getpeername())");
  }
  return std::string(inet_ntoa(addr.sin_addr));
}

util::StatusOr<unsigned short> CommunicatingSocket::getForeignPort() {
  sockaddr_in addr;
  unsigned int addr_len = sizeof(addr);
  if (getpeername(sockDesc_, (sockaddr *)&addr, (socklen_t *)&addr_len) < 0) {
    return util::Status(util::StatusCode::kUnknown,
                        "Fetch of foreign port failed (getpeername())");
  }
  return ntohs(addr.sin_port);
}

}  // namespace socket
}  // namespace da
