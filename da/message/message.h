#ifndef __INCLUDED_DA_MESSAGE_MESSAGE_H_
#define __INCLUDED_DA_MESSAGE_MESSAGE_H_

#include <ostream>
#include <string>

#include <da/util/util.h>

namespace da {
namespace message {

class Message {
 public:
  Message(int process_id, int message_id, bool ack = false)
      : process_id_(process_id), message_id_(message_id), ack_(ack) {}

  Message(const char* message) {
    process_id_ = util::stringToInteger(message);
    message_id_ = util::stringToInteger(message + sizeof(int));
    ack_ = util::stringToBool(message + 2 * sizeof(int));
  }

  int getProcessId() const { return process_id_; }
  int getMessageId() const { return message_id_; }
  bool isAck() const { return ack_; }

  std::string toString() const {
    using namespace std::string_literals;
    std::string message = ""s;
    message += util::integerToString(process_id_);
    message += util::integerToString(message_id_);
    message += util::boolToString(ack_);
    return message;
  }

  std::string toAckString() const {
    using namespace std::string_literals;
    std::string message = ""s;
    message += util::integerToString(process_id_);
    message += util::integerToString(message_id_);
    message += util::boolToString(!ack_);
    return message;
  }

 private:
  friend std::ostream& operator<<(std::ostream&, const Message&);

  int process_id_;
  int message_id_;
  bool ack_;
};

}  // namespace message
}  // namespace da

#endif  // __INCLUDED_DA_MESSAGE_MESSAGE_H_
