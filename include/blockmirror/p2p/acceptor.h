#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include <blockmirror/p2p/channel.h>
#include <boost/asio.hpp>

namespace blockmirror {
namespace p2p {

class Acceptor {
 public:
  Acceptor(boost::asio::io_context& ioc, unsigned short port);

 public:
  void startAccept();

 private:
  void handleAccept(const boost::system::error_code& e);

 private:
  boost::asio::io_context& _ioContext;
  boost::asio::ip::tcp::acceptor _acceptor;
  boost::shared_ptr<Channel> _newChannel;
};

}  // namespace p2p
}  // namespace blockmirror

#endif