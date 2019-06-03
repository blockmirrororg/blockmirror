
#include <blockmirror/p2p/channel.h>
#include <blockmirror/p2p/connector.h>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <iostream>

namespace blockmirror {
namespace p2p {

Connector::Connector(boost::asio::io_context& ioc, const char* ip,
                     unsigned short port)
    : endpoint_(boost::asio::ip::address::from_string(ip), port),
      _timer(ioc),
      _ioContext(ioc) {}

void Connector::start(bool now) {
  _socket.reset(new boost::asio::ip::tcp::socket(_ioContext));
  if (now) {
    _socket->async_connect(
        endpoint_, boost::bind(&Connector::handleConnect, shared_from_this(),
                               boost::asio::placeholders::error));
  } else {
    _timer.expires_from_now(boost::posix_time::seconds(5));
    _timer.async_wait(boost::bind(&Connector::handleTimer, shared_from_this()));
  }
}

void Connector::handleConnect(const boost::system::error_code& ec) {
  if (!ec) {
    boost::shared_ptr<Channel> channel = boost::make_shared<Channel>(_socket, _ioContext);
    channel->setConnector(shared_from_this());
    channel->start();
  } else {
    _socket->close();
    _timer.expires_from_now(boost::posix_time::seconds(10));
    _timer.async_wait(boost::bind(&Connector::handleTimer, shared_from_this()));
  }
}

void Connector::handleTimer() {
  _socket->async_connect(
      endpoint_, boost::bind(&Connector::handleConnect, shared_from_this(),
                             boost::asio::placeholders::error));
}

}  // namespace p2p
}  // namespace blockmirror