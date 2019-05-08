#ifndef REQUEST_ACK_HANDLER_H
#define REQUEST_ACK_HANDLER_H

#include <blockmirror/p2p/binary_stream.h>
#include "binary_stream.h"

namespace blockmirror {
namespace p2p {

class App_Request_Handler {
 public:
  virtual void handle_request() = 0;
};

class App_Async_Ack_Handler {
 public:
  virtual void handle_ack(const Message& request) = 0;
  virtual void handle_timeout(const Message& request) = 0;
};

}  // namespace p2p
}  // namespace blockmirror

#endif