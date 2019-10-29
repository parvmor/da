#include <da/socket/udp_socket.h>

#include <errno.h>

#include <da/socket/communicating_socket.h>

namespace da {
namespace socket {

UDPSocket::UDPSocket() throw() : CommunicatingSocket(SOCK_DGRAM, IPPROTO_UDP) {
  setBroadcast();
}

UDPSocket::UDPSocket(unsigned short localPort) throw()
    : CommunicatingSocket(SOCK_DGRAM, IPPROTO_UDP) {
  setLocalPort(localPort);
  setBroadcast();
}

UDPSocket::UDPSocket(const std::string &localAddress,
                     unsigned short localPort) throw()
    : CommunicatingSocket(SOCK_DGRAM, IPPROTO_UDP) {
  setLocalAddressAndPort(localAddress, localPort);
  setBroadcast();
}

UDPSocket::UDPSocket(const std::string &localAddress, unsigned short localPort,
                     struct timeval tv) throw()
    : CommunicatingSocket(SOCK_DGRAM, IPPROTO_UDP) {
  const auto status = setLocalAddressAndPort(localAddress, localPort);
  if (!status.ok()) {
    throw util::RuntimeStatusError(status);
  }
  setBroadcast();
  setTimeout(tv);
}

void UDPSocket::setTimeout(struct timeval tv) {
  setsockopt(sockDesc_, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
}

void UDPSocket::setBroadcast() {
  // If this fails, we'll hear about it when we try to send. This will allow
  // system that cannot broadcast to continue if they don't plan to broadcast.
  int broadcastPermission = 1;
  setsockopt(sockDesc_, SOL_SOCKET, SO_BROADCAST, (void *)&broadcastPermission,
             sizeof(broadcastPermission));
}

util::Status UDPSocket::disconnect() {
  sockaddr_in nullAddr;
  memset(&nullAddr, 0, sizeof(nullAddr));
  nullAddr.sin_family = AF_UNSPEC;
  // Try to disconnect.
  if (::connect(sockDesc_, (sockaddr *)&nullAddr, sizeof(nullAddr)) < 0) {
    if (errno != EAFNOSUPPORT) {
      return util::Status(util::StatusCode::kUnknown,
                          "Disconnect failed (connect())");
    }
  }
  return util::Status();
}

util::Status UDPSocket::sendTo(const void *buffer, int bufferLen,
                               const std::string &foreignAddress,
                               unsigned short foreignPort) {
  sockaddr_in destAddr;
  const auto status = fillAddr(foreignAddress, foreignPort, destAddr);
  if (!status.ok()) {
    return status;
  }
  // Write out the whole buffer as a single message.
  if (::sendto(sockDesc_, (void *)buffer, bufferLen, 0, (sockaddr *)&destAddr,
               sizeof(destAddr)) != bufferLen) {
    return util::Status(util::StatusCode::kUnknown, "Send failed (sendto())");
  }
  return util::Status();
}

util::StatusOr<int> UDPSocket::recvFrom(void *buffer, int bufferLen,
                                        std::string &sourceAddress,
                                        unsigned short &sourcePort) {
  sockaddr_in clntAddr;
  socklen_t addrLen = sizeof(clntAddr);
  int rtn;
  if ((rtn = ::recvfrom(sockDesc_, (void *)buffer, bufferLen, 0,
                        (sockaddr *)&clntAddr, (socklen_t *)&addrLen)) < 0) {
    return util::Status(util::StatusCode::kUnknown,
                        "Receive failed (recvfrom())");
  }
  sourceAddress = inet_ntoa(clntAddr.sin_addr);
  sourcePort = ntohs(clntAddr.sin_port);
  return rtn;
}

util::Status UDPSocket::setMulticastTTL(unsigned char multicastTTL) {
  if (setsockopt(sockDesc_, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&multicastTTL,
                 sizeof(multicastTTL)) < 0) {
    return util::Status(util::StatusCode::kUnknown,
                        "Multicast TTL set failed (setsockopt())");
  }
  return util::Status();
}

util::Status UDPSocket::joinGroup(const std::string &multicastGroup) {
  struct ip_mreq multicastRequest;
  multicastRequest.imr_multiaddr.s_addr = inet_addr(multicastGroup.c_str());
  multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
  if (setsockopt(sockDesc_, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                 (void *)&multicastRequest, sizeof(multicastRequest)) < 0) {
    return util::Status(util::StatusCode::kUnknown,
                        "Multicast group join failed (setsockopt())");
  }
  return util::Status();
}

util::Status UDPSocket::leaveGroup(const std::string &multicastGroup) {
  struct ip_mreq multicastRequest;
  multicastRequest.imr_multiaddr.s_addr = inet_addr(multicastGroup.c_str());
  multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
  if (setsockopt(sockDesc_, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                 (void *)&multicastRequest, sizeof(multicastRequest)) < 0) {
    return util::Status(util::StatusCode::kUnknown,
                        "Multicast group leave failed (setsockopt())");
  }
  return util::Status();
}

}  // namespace socket
}  // namespace da
