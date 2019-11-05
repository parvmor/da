// The following implementation has been inspired from:
// http://cs.ecs.baylor.edu/~donahoo/practical/CSockets/practical/
#ifndef __INCLUDED_DA_SOCKET_SOCKET_H_
#define __INCLUDED_DA_SOCKET_SOCKET_H_

#include <arpa/inet.h>
#include <da/util/status.h>
#include <da/util/statusor.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <string>

namespace da {
namespace socket {

// Fills the socket addr structure with the mentioned address and port.
util::Status fillAddr(const std::string& address, unsigned short port,
                      sockaddr_in& addr);

// A base class representing a socket.
class Socket {
 public:
  // Closes and deallocates the socket.
  ~Socket();

  util::StatusOr<std::string> getLocalAddress();

  util::StatusOr<unsigned short> getLocalPort();

  // Sets the local port to the specified port and local address to any
  // interface.
  util::Status setLocalPort(unsigned short localPort);

  // Sets the local address and port to the specified parameters. If the port is
  // omitted then a random port is selected.
  util::Status setLocalAddressAndPort(const std::string& localAddress,
                                      unsigned short localPort = 0);

  // Resolves the specified service for the specified protocol to the
  // corresponding port number in host byte order.
  static unsigned short resolveService(const std::string& service,
                                       const std::string& protocol = "udp");

 protected:
  Socket(int type, int protocol) throw();
  Socket(int sockDesc);
  // The socket descriptor.
  int sockDesc_;

 private:
  // Prevents user from trying to use value semantics on this object.
  Socket(const Socket& sock);

  void operator=(const Socket& sock);
};

}  // namespace socket
}  // namespace da

#endif  // __INCLUDED_DA_SOCKET_SOCKET_H_
