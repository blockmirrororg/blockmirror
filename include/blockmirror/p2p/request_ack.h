#ifndef REQUEST_ACK_H
#define REQUEST_ACK_H

#include <blockmirror/p2p/message_handler.h>
#include <blockmirror/p2p/request_ack_handler.h>

namespace blockmirror {
namespace p2p {

class RequestHandler : public MessageHandler {
 public:
  RequestHandler(App_Request_Handler&);
  virtual void handle_message();

 private:
  App_Request_Handler& app_handler_;
};

// ------------------------------------------------------------------------------------
class AsyncAckHandler : public MessageHandler {
 public:
  AsyncAckHandler(App_Async_Ack_Handler&);
  virtual void handle_message();
  void handle_timeout(const Message& request);

 private:
  App_Async_Ack_Handler& app_handler_;
};

}  // namespace p2p
}  // namespace blockmirror

#endif