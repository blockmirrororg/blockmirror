
#include <blockmirror/p2p/channel.h>
#include <blockmirror/p2p/channel_manager.h>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/bind.hpp>

namespace blockmirror {
namespace p2p {

Channel::Channel(boost::shared_ptr<boost::asio::ip::tcp::socket>& socket, boost::asio::io_context& ioc)
    : _socket(socket), _timer(ioc), _strand(ioc) {}

// ChannelManager引用的是weak_ptr，那么Channel什么时候会析构，当没有处于异步recv或者send状态中的operation，即没有shared_from_this()
// hold住shared_ptr
Channel::~Channel() {
  ChannelManager::get().removeChannel(_channelId);
  if (_connector) {
    _connector->start(false);
  }
}

void Channel::start() {
  ChannelManager::get().addChannel(shared_from_this());

	_current = std::time(0);
	_timer.expires_from_now(boost::posix_time::seconds(10));
	_timer.async_wait(boost::bind(&Channel::handleTimer, shared_from_this()));

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

    if (header.length > MESSAGE_MAX_LENGTH)
    {
      close();
      return;
    }

    boost::asio::async_read(
        *_socket, _binaryBuf, boost::asio::transfer_exactly(header.length),
        boost::bind(&Channel::handleReadBody, shared_from_this(),
                    boost::asio::placeholders::error));
  }
}

// 收到的网络消息在这里处理
class MessageVisitor : public boost::static_visitor<> {
 public:
  MessageVisitor(Channel& channel) : _channel(channel) {}

  void operator()(const MsgHeartbeat& msg) const
	{
		_channel._current = std::time(0);
	}

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

		// 接着收下一个包
    boost::asio::async_read(
        *_socket, _binaryBuf,
        boost::asio::transfer_exactly(sizeof(MessageHeader)),
        boost::bind(&Channel::handleReadHeader, shared_from_this(),
                    boost::asio::placeholders::error));
  }
}

void Channel::close() {
  boost::system::error_code ignored_ec;
  _socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
}

void Channel::send(const Message& msg) {
  boost::asio::streambuf binaryBuf;
  boost::archive::binary_oarchive archive(binaryBuf);
  archive << msg;
  boost::asio::async_write(
      *_socket, binaryBuf,
      _strand.wrap(boost::bind(&Channel::handleWrite, shared_from_this(),
                               boost::asio::placeholders::error)));
}

void Channel::handleWrite(const boost::system::error_code& e) {
  if (!e) {
    // do nothing
  }
}

void Channel::handleTimer() {
  if (_connector) {
    send(MsgHeartbeat());  // 每隔10秒向服务端发送心跳包
  } else {
    if (std::time(0) - _current > 15) {
			// 一定时间内没收到客户端的心跳包，认为该连接已僵死，该间隔可以设置得大一点
      close();
      return;
    }
  }

  _timer.expires_from_now(boost::posix_time::seconds(10));
  _timer.async_wait(boost::bind(&Channel::handleTimer, shared_from_this()));
}

}  // namespace p2p
}  // namespace blockmirror