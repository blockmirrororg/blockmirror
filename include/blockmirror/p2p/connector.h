#ifndef CONNECTOR_H
#define CONNECTOR_H

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace blockmirror {
namespace p2p {

class Connector : public boost::enable_shared_from_this<Connector> {
 public:
  Connector(boost::asio::io_context& ioc, const char* ip, unsigned short port,
            unsigned char remote_type = 0, unsigned char local_type = 0);

  void start(bool now = true);

 private:
  void handle_connect(const boost::system::error_code& ec);
  void handle_write(const boost::system::error_code& ec);
  void handle_timer();

 private:
  boost::asio::ip::tcp::socket socket_;
  boost::asio::ip::tcp::endpoint endpoint_;

  boost::asio::deadline_timer timer_;
  boost::asio::io_context& io_context_;
  unsigned char remote_type_;
  unsigned char local_type_;
};

}  // namespace p2p
}  // namespace blockmirror

#endif