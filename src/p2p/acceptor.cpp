
#include <blockmirror/p2p/acceptor.h>
#include <boost/bind.hpp>

namespace blockmirror {
namespace p2p {

Acceptor::Acceptor(boost::asio::io_context& ioc, unsigned short port)
    : io_context_(ioc),
      acceptor_(io_context_, boost::asio::ip::tcp::endpoint(
                                 boost::asio::ip::tcp::v4(), port)) {}

void Acceptor::start_accept() {
  new_channel_.reset(new Channel(io_context_));
  acceptor_.async_accept(new_channel_->socket(),
                         boost::bind(&Acceptor::handle_accept, this,
                                     boost::asio::placeholders::error));
}

void Acceptor::handle_accept(const boost::system::error_code& e) {
  if (!e) {
    // new_connection_->start();
  }

  start_accept();
}

}  // namespace p2p
}  // namespace blockmirror