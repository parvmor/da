#include <da/message/message.h>

#include <ostream>

namespace da {
namespace message {

std::ostream& operator<<(std::ostream& stream, const Message& msg) {
  return stream << "Message{ process_id: " << msg.process_id_
                << ", message_id: " << msg.message_id_ << ", ack: " << msg.ack_
                << " }";
}

}  // namespace message
}  // namespace da
