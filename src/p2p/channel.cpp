
#include <blockmirror/p2p/channel.h>
#include <blockmirror/p2p/channel_manager.h>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/bind.hpp>

namespace blockmirror {
namespace p2p {

Channel::Channel(boost::shared_ptr<boost::asio::ip::tcp::socket>& socket) : _socket(socket) {}

// ChannelManager引用的是weak_ptr，那么Channel什么时候会析构，当没有处于异步recv或者send状态中的operation，即没有shared_from_this() hold住shared_ptr
Channel::~Channel()
{
  ChannelManager::get().removeChannel(_channelId);
  /*if (_connector)
  {
    _connector
  }*/
}

void Channel::start() {
  ChannelManager::get().addChannel(shared_from_this());

  boost::asio::async_read(
      *_socket, _binaryBuf, boost::asio::transfer_exactly(sizeof(MessageHeader)),
      boost::bind(&Channel::handleReadHeader, shared_from_this(),
                  boost::asio::placeholders::error));
}

void Channel::handleReadHeader(const boost::system::error_code& e) {
  if (!e) {
    MessageHeader header;
    boost::archive::binary_iarchive archive(_binaryBuf);
    archive >> header;

    boost::asio::async_read(
        *_socket, _binaryBuf, boost::asio::transfer_exactly(header.length),
        boost::bind(&Channel::handleReadBody, shared_from_this(),
                    boost::asio::placeholders::error));
  }
}

class MessageVisitor : public boost::static_visitor<> {
 public:
  MessageVisitor(Channel& channel) : _channel(channel) {}

  void operator()(const MsgHeartbeat& msg) const {}

  void operator()(const MsgBroadcastBlock& msg) const {}

 private:
  Channel& _channel;
};

void Channel::handleReadBody(const boost::system::error_code& e) {
  if (!e) {
    Message message;
    boost::archive::binary_iarchive archive(_binaryBuf);
    archive >> message;
    boost::apply_visitor(MessageVisitor(*this), message);

    start();
  }
}

void Channel::close() {
  boost::system::error_code ignored_ec;
  _socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
}

void Channel::send(const Message& msg) {
  boost::archive::binary_oarchive archive(_binaryBuf);
  archive << msg;
  boost::asio::async_write(
      *_socket, _binaryBuf,
      boost::bind(&Channel::handleWrite, shared_from_this(),
                  boost::asio::placeholders::error));
}

void Channel::handleWrite(const boost::system::error_code& e) {
  if (!e) {
  }
}

}  // namespace p2p
}  // namespace blockmirror