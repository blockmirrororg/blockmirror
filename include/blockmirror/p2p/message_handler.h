#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

namespace blockmirror {
namespace p2p {

class MessageHandler {
 public:
  virtual ~MessageHandler() {}

 public:
  virtual void handle_message() = 0;
};  // namespace p2pclassMessageHandler

}  // namespace p2p
}  // namespace blockmirror

#endif