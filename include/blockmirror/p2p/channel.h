#pragma once

#include <blockmirror/p2p/message.h>
#include <blockmirror/p2p/connector.h>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

namespace blockmirror {
namespace p2p {

class Channel : public boost::enable_shared_from_this<Channel>,
                private boost::noncopyable {
 public:
  explicit Channel(boost::shared_ptr<boost::asio::ip::tcp::socket>& socket);
  ~Channel();

 public:
  void id(int id) { _channelId = id; }
  int id() { return _channelId; }
  //boost::asio::ip::tcp::socket& socket() { return _socket; }
  void start();
  void close();
  void send(const Message& message);
  void setConnector(boost::shared_ptr<Connector> connector) {
    _connector_ = connector;
  }

 private:
  void handleReadHeader(const boost::system::error_code& e);
  void handleReadBody(const boost::system::error_code& e);
  void handleWrite(const boost::system::error_code& e);

 private:
   boost::shared_ptr<Connector> _connector_;
  boost::asio::streambuf _binaryBuf;
  boost::shared_ptr<boost::asio::ip::tcp::socket> _socket;
  int _channelId;
};

}  // namespace p2p
}  // namespace blockmirror