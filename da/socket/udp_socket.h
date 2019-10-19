#ifndef __INCLUDED_DA_SOCKET_UDP_SOCKET_H_
#define __INCLUDED_DA_SOCKET_UDP_SOCKET_H_

#include <string>

#include <da/socket/communicating_socket.h>

namespace da {
namespace socket {

class UDPSocket : public CommunicatingSocket {
 public:
  UDPSocket() throw();

  UDPSocket(unsigned short localPort) throw();

  UDPSocket(const std::string& localAddress, unsigned short localPort) throw();

  // Unset foreign address and port.
  util::Status disconnect();

  // Send the given buffer as a UDP datagram to the specified address/port.
  util::Status sendTo(const void* buffer, int bufferLen,
                      const std::string& foreignAddress,
                      unsigned short foreignPort);

  // Read read up to bufferLen bytes data from this socket.  The given buffer is
  // where the data will be placed.
  util::StatusOr<int> recvFrom(void* buffer, int bufferLen,
                               std::string& sourceAddress,
                               unsigned short& sourcePort);

  // Set the multicast TTL.
  util::Status setMulticastTTL(unsigned char multicastTTL);

  // Join the specified multicast group.
  util::Status joinGroup(const std::string& multicastGroup);

  // Leave the specified multicast group.
  util::Status leaveGroup(const std::string& multicastGroup);

 private:
  void setBroadcast();
};

}  // namespace socket
}  // namespace da

#endif  // __INCLUDED_DA_SOCKET_UDP_SOCKET_H_
