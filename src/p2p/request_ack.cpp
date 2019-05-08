
#include <blockmirror/p2p/request_ack.h>

namespace blockmirror {
namespace p2p {

RequestHandler::RequestHandler(App_Request_Handler& app_handler)
    : app_handler_(app_handler) {}

void RequestHandler::handle_message() { app_handler_.handle_request(); }

AsyncAckHandler::AsyncAckHandler(App_Async_Ack_Handler& app_handler)
    : app_handler_(app_handler) {}

void AsyncAckHandler::handle_message() {}

void AsyncAckHandler::handle_timeout(const Message& request) {
  app_handler_.handle_timeout(request);
}

}  // namespace p2p
}  // namespace blockmirror