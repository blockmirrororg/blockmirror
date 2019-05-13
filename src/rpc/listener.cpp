
#include <blockmirror/rpc/listener.h>
#include <blockmirror/rpc/session.h>

namespace blockmirror {
namespace rpc {

Listener::Listener(boost::asio::io_context& ioc, tcp::endpoint endpoint, blockmirror::chain::Context &context)
    : acceptor_(ioc), socket_(ioc), _context(context) {
  boost::system::error_code ec;

  acceptor_.open(endpoint.protocol(), ec);
  if (ec) {
    return;
  }

  acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
  if (ec) {
    return;
  }

  acceptor_.bind(endpoint, ec);
  if (ec) {
    return;
  }

  acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
  if (ec) {
    return;
  }
}

void Listener::run() {
  if (!acceptor_.is_open()) return;
  do_accept();
}

void Listener::do_accept() {
  acceptor_.async_accept(socket_,
                         std::bind(&Listener::on_accept, this,
                                   std::placeholders::_1));
}

void Listener::on_accept(boost::system::error_code ec) {
  if (!ec) {
    std::make_shared<Session>(std::move(socket_), _context)->run();
  }

  do_accept();
}

}  // namespace rpc
}  // namespace blockmirror