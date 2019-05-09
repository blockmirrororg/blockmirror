
#include <blockmirror/p2p/connector.h>
#include <boost/bind.hpp>
#include <iostream>

namespace blockmirror {
namespace p2p {

Connector::Connector(boost::asio::io_context& ioc, const char* ip,
                     unsigned short port, unsigned char remote_type,
                     unsigned char local_type)
    : socket_(ioc),
      // resolver_(ioc),
      endpoint_(boost::asio::ip::address::from_string(ip), port),
      timer_(ioc),
      io_context_(ioc),
      remote_type_(remote_type),
      local_type_(local_type) {}

void Connector::start(bool now) {
  if (now) {
     socket_.async_connect(endpoint_, boost::bind(&Connector::handle_connect,
     this, boost::asio::placeholders::error));

    //boost::asio::ip::tcp::resolver::query query("www.boost.org", "http");
    //resolver_.async_resolve(query,
    //                        boost::bind(&Connector::handle_resolve, this,
    //                                    boost::asio::placeholders::error,
    //                                    boost::asio::placeholders::iterator));
  } else {
    timer_.expires_from_now(boost::posix_time::seconds(5));
    timer_.async_wait(boost::bind(&Connector::handle_timer, this));
  }
}

void Connector::handle_connect(const boost::system::error_code& ec) {
  if (!ec) {
    // build a channel
  } else {
    socket_.close();
    timer_.expires_from_now(boost::posix_time::seconds(5));
    timer_.async_wait(boost::bind(&Connector::handle_timer, this));
  }
}

void Connector::handle_timer() {
   socket_.async_connect(endpoint_, boost::bind(&Connector::handle_connect,
   shared_from_this(), boost::asio::placeholders::error));
}

//void Connector::handle_resolve(
//    const boost::system::error_code& err,
//    boost::asio::ip::tcp::resolver::iterator endpoint_iterator) {
//  if (!err) {
//    boost::asio::async_connect(socket_, endpoint_iterator,
//                               boost::bind(&Connector::handle_connect, this,
//                                           boost::asio::placeholders::error));
//  } else {
//    std::cout << "Error: " << err.message() << "\n";
//  }
//}

}  // namespace p2p
}  // namespace blockmirror