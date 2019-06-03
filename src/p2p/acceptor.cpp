
#include <blockmirror/p2p/acceptor.h>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

namespace blockmirror {
namespace p2p {

Acceptor::Acceptor(boost::asio::io_context& ioc, unsigned short port)
    : _ioContext(ioc),
      _acceptor(_ioContext, boost::asio::ip::tcp::endpoint(
                                boost::asio::ip::tcp::v4(), port)) {}

void Acceptor::startAccept() {
  boost::shared_ptr<boost::asio::ip::tcp::socket> sock =
      boost::make_shared<boost::asio::ip::tcp::socket>(_ioContext);
  _newChannel.reset(new Channel(sock, _ioContext));
  _acceptor.async_accept(_newChannel->socket(),
                         boost::bind(&Acceptor::handleAccept, this,
                                     boost::asio::placeholders::error));
}

void Acceptor::handleAccept(const boost::system::error_code& e) {
  if (!e) {
    _newChannel->start();
  }

  startAccept();
}

}  // namespace p2p
}  // namespace blockmirror