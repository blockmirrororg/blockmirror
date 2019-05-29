#ifndef LISTENER_H
#define LISTENER_H

#include <blockmirror/chain/context.h>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/config.hpp>

namespace blockmirror {
namespace rpc {

using tcp = boost::asio::ip::tcp;

class Listener {
  tcp::acceptor acceptor_;
  tcp::socket socket_;
  blockmirror::chain::Context &_context;
  boost::asio::io_context &_ioc;

 public:
  Listener(boost::asio::io_context &ioc, tcp::endpoint endpoint,
           blockmirror::chain::Context &ctx);

  void run();
  void do_accept();
  void on_accept(boost::system::error_code ec/*, tcp::socket socket*/);
};

}  // namespace rpc
}  // namespace blockmirror

#endif