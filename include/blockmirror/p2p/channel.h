#pragma once

#include <blockmirror/p2p/connector.h>
#include <blockmirror/p2p/message.h>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

namespace blockmirror {
namespace p2p {

class Channel : public boost::enable_shared_from_this<Channel>,
                private boost::noncopyable {
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

  void sendBroadcastBlock(const uint64_t& height, const Hash256& hash);
  void sendGenerateBlock(blockmirror::chain::BlockPtr block);

  void handleMessage(const MsgHeartbeat& msg) { _current = std::time(0); }
  void handleMessage(const MsgHello& msg);
  void handleMessage(const MsgSyncReq& msg);
  void handleMessage(const MsgBlock& msg);
  void handleMessage(const MsgGenerateBlock& msg);
  void handleMessage(const MsgBroadcastBlock& msg);

 private:
  void handleReadHeader(const boost::system::error_code& e);
  void handleReadBody(const boost::system::error_code& e);
  void handleTimer();
  void emplaceTimer();
  void sendHello();

 private:
  boost::shared_ptr<Connector> _connector;
  boost::asio::streambuf _binaryBuf;  // 用于接收
  boost::shared_ptr<boost::asio::ip::tcp::socket> _socket;
  int _channelId;
  boost::asio::deadline_timer _timer;
  std::time_t _current;
  boost::asio::io_context::strand _strand;
  boost::array<char, sizeof(MessageHeader)> _header;
};

using ChannelPtr = boost::shared_ptr<Channel>;
using ChannelWPtr = boost::weak_ptr<Channel>;

}  // namespace p2p
}  // namespace blockmirror