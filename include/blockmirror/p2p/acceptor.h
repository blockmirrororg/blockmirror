#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include <boost/asio.hpp>
#include <blockmirror/p2p/channel.h>

namespace blockmirror {
namespace p2p {

class Acceptor {
 public:
  Acceptor(boost::asio::io_context &ioc, unsigned short port);

 public:
  void startAccept();
  void run();

 private:
  void handleAccept(const boost::system::error_code &e);

 private:
  boost::asio::io_context &io_context_;
  boost::asio::ip::tcp::acceptor acceptor_;
  ChannelPtr _newChannel;
};

}  // namespace p2p
}  // namespace blockmirror

#endif