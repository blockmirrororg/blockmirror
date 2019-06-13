
#include <blockmirror/p2p/channel.h>
#include <blockmirror/p2p/channel_manager.h>
#include <boost/bind.hpp>
#include <blockmirror/serialization/binary_iarchive.h>
#include <blockmirror/serialization/binary_oarchive.h>
#include <boost/asio/bind_executor.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

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
	// _timer.expires_from_now(boost::posix_time::seconds(10));
	// _timer.async_wait(boost::bind(&Channel::handleTimer, shared_from_this()));
  emplaceTimer(); // 安置一个定时器

  boost::asio::async_read(
      *_socket, _binaryBuf, boost::asio::transfer_exactly(sizeof(MessageHeader)),
      boost::bind(&Channel::handleReadHeader, shared_from_this(),
                  boost::asio::placeholders::error));
}

void Channel::handleReadHeader(const boost::system::error_code& e) {
  if (!e) {
    MessageHeader header;
    blockmirror::serialization::AsioStream stream(_binaryBuf);
    blockmirror::serialization::BinaryIArchive<blockmirror::serialization::AsioStream> archive(stream);
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

  void operator()(const MsgGenerateBlock& msg) const {}

 private:
  Channel& _channel;
};

void Channel::handleReadBody(const boost::system::error_code& e) {
  if (!e) {
    Message message;
    blockmirror::serialization::AsioStream stream(_binaryBuf);
    blockmirror::serialization::BinaryIArchive<blockmirror::serialization::AsioStream> archive(stream);
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
  boost::shared_ptr<boost::asio::streambuf> sb = boost::make_shared<boost::asio::streambuf>();
  // binaryBuf.commit(sizeof(MessageHeader)); // 先预留一个头包头的位置
  blockmirror::serialization::AsioStream stream(*sb);
  blockmirror::serialization::BinaryOArchive<blockmirror::serialization::AsioStream> archive(stream);
  archive << msg;

  // MessageHeader header;
  // header.length = binaryBuf.size() - sizeof(MessageHeader);
  // binaryBuf.setp(binaryBuf.pbase(), binaryBuf.epptr()); // 设置三个写指针为最初的状态
  // archive << header;
  // binaryBuf.pbump(header.length); // header.length此时等于binaryBuf.egptr() - binaryBuf.pptr()

  boost::shared_ptr<boost::asio::streambuf> sb1 = boost::make_shared<boost::asio::streambuf>();;
  blockmirror::serialization::AsioStream stream1(*sb1);
  blockmirror::serialization::BinaryOArchive<blockmirror::serialization::AsioStream> archive1(stream1);
  MessageHeader header;
  header.length = sb->size();
  archive1 << header;

  std::vector<boost::asio::const_buffer> v;
  v.push_back(sb1->data()); // 先发送头
  v.push_back(sb->data());

  // boost::asio::async_write(
  //     *_socket, v,
  //     _strand.wrap(boost::bind(&Channel::handleWrite, shared_from_this(),
  //                              boost::asio::placeholders::error)));

  ChannelPtr channel(shared_from_this());
  boost::asio::async_write(*_socket, v,
                           boost::asio::bind_executor(_strand, [channel, sb, sb1](boost::system::error_code ec, std::size_t w) {
                             // do nothing
                           }));
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

  emplaceTimer();
}

void Channel::emplaceTimer() {
  _timer.expires_from_now(boost::posix_time::seconds(10));
  ChannelWPtr weak(shared_from_this());
  _timer.async_wait([weak](const boost::system::error_code&) {
    auto channel = weak.lock();
    if(!channel) {
      return;
    }

    channel->handleTimer();
  });
}

}  // namespace p2p
}  // namespace blockmirror