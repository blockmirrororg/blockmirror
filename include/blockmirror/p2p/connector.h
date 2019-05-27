#ifndef CONNECTOR_H
#define CONNECTOR_H

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <blockmirror/p2p/channel.h>

namespace blockmirror {
namespace p2p {

class Connector : public boost::enable_shared_from_this<Connector> {
 public:
  Connector(boost::asio::io_context& ioc, const char* ip, unsigned short port);

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
  ChannelPtr _newChannel;
};

}  // namespace p2p
}  // namespace blockmirror

#endif