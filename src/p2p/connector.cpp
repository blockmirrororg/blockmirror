
#include <blockmirror/p2p/connector.h>
#include <boost/bind.hpp>
#include <iostream>

namespace blockmirror {
namespace p2p {

Connector::Connector(boost::asio::io_context& ioc, const char* ip,
                     unsigned short port)
    : socket_(ioc),
      endpoint_(boost::asio::ip::address::from_string(ip), port),
      timer_(ioc),
      io_context_(ioc){}

void Connector::start(bool now) {

  if (now) {
    socket_.async_connect(
        endpoint_, boost::bind(&Connector::handle_connect, shared_from_this(),
                               boost::asio::placeholders::error));
  } else {
    timer_.expires_from_now(boost::posix_time::seconds(5));
    timer_.async_wait(
        boost::bind(&Connector::handle_timer, shared_from_this()));
  }
}

void Connector::handle_connect(const boost::system::error_code& ec) {
  if (!ec) {
    // build a channel
  } else {
    socket_.close();
    timer_.expires_from_now(boost::posix_time::seconds(10));
    timer_.async_wait(
        boost::bind(&Connector::handle_timer, shared_from_this()));
  }
}

void Connector::handle_timer() {
  socket_.async_connect(
      endpoint_, boost::bind(&Connector::handle_connect, shared_from_this(),
                             boost::asio::placeholders::error));
}

}  // namespace p2p
}  // namespace blockmirror