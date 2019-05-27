
#include <blockmirror/p2p/acceptor.h>
#include <boost/bind.hpp>

namespace blockmirror {
namespace p2p {

Acceptor::Acceptor(boost::asio::io_context& ioc, unsigned short port)
    : io_context_(ioc),
      acceptor_(io_context_, boost::asio::ip::tcp::endpoint(
                                 boost::asio::ip::tcp::v4(), port)) {}

void Acceptor::startAccept() {
  _newChannel.reset(new Channel(io_context_));
  acceptor_.async_accept(_newChannel->socket(),
                         boost::bind(&Acceptor::handleAccept, this,
                                     boost::asio::placeholders::error));
}

void Acceptor::handleAccept(const boost::system::error_code& e) {
  if (!e) {
    _newChannel->start();
  }

  startAccept();
}

void Acceptor::run() {
  if (!acceptor_.is_open()) return;
  startAccept();
}

}  // namespace p2p
}  // namespace blockmirror