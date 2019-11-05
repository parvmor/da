#ifndef __INCLUDED_DA_SOCKET_COMMUNICATING_SOCKET_H_
#define __INCLUDED_DA_SOCKET_COMMUNICATING_SOCKET_H_

#include <da/socket/socket.h>

#include <string>

namespace da {
namespace socket {

class CommunicatingSocket : public Socket {
 public:
  // Establish a socket connection with the given foreign address and port.
  util::Status connect(const std::string& foreignAddress,
                       unsigned short foreignPort);

  // Write the given buffer to this socket. Call connect() before calling
  // send().
  util::Status send(const void* buffer, int bufferLen);

  // Read into the given buffer up to bufferLen bytes data from this socket.
  // Call connect() before calling recv().
  util::StatusOr<int> recv(void* buffer, int bufferLen);

  // Get the foreign address. Call connect() before calling recv().
  util::StatusOr<std::string> getForeignAddress();

  // Get the foreign port. Call connect() before calling recv().
  util::StatusOr<unsigned short> getForeignPort();

 protected:
  CommunicatingSocket(int type, int protocol) throw();
  CommunicatingSocket(int sockDesc);
};

}  // namespace socket
}  // namespace da

#endif  // __INCLUDED_DA_SOCKET_COMMUNICATING_SOCKET_H_
