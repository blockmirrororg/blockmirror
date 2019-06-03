#pragma once

#include <blockmirror/p2p/connector.h>
#include <blockmirror/p2p/message.h>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

namespace blockmirror {
namespace p2p {

class Channel : public boost::enable_shared_from_this<Channel>,
                private boost::noncopyable {
  friend class MessageVisitor;

 public:
  explicit Channel(boost::shared_ptr<boost::asio::ip::tcp::socket>& socket,
                   boost::asio::io_context& ioc);
  ~Channel();

 public:
  void id(int id) { _channelId = id; }
  int id() { return _channelId; }
  boost::asio::ip::tcp::socket& socket() { return *_socket; }
  void start();
  void close();
  void send(const Message& message);
  void setConnector(boost::shared_ptr<Connector> connector) {
    _connector = connector;
  }

 private:
  void handleReadHeader(const boost::system::error_code& e);
  void handleReadBody(const boost::system::error_code& e);
  void handleWrite(const boost::system::error_code& e);
  void handleTimer();

 private:
  boost::shared_ptr<Connector> _connector;
  boost::asio::streambuf _binaryBuf;  // 用于接收
  boost::shared_ptr<boost::asio::ip::tcp::socket> _socket;
  int _channelId;
  boost::asio::deadline_timer _timer;
  std::time_t _current;
};

}  // namespace p2p
}  // namespace blockmirror