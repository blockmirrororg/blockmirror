#ifndef LISTENER_H
#define LISTENER_H

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/config.hpp>

namespace blockmirror {
namespace rpc {

using tcp = boost::asio::ip::tcp;

class Listener : public std::enable_shared_from_this<Listener> {
  tcp::acceptor acceptor_;
  tcp::socket socket_;
  std::shared_ptr<std::string const> doc_root_;

 public:
  Listener(boost::asio::io_context& ioc, tcp::endpoint endpoint);

  void run();
  void do_accept();
  void on_accept(boost::system::error_code ec);
};

}  // namespace rpc
}  // namespace blockmirror

#endif