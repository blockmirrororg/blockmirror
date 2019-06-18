
#include <blockmirror/p2p/channel.h>
#include <blockmirror/p2p/channel_manager.h>
#include <blockmirror/serialization/binary_iarchive.h>
#include <blockmirror/serialization/binary_oarchive.h>
#include <blockmirror/server.h>
#include <blockmirror/store/mongo_store.h>
#include <boost/asio/bind_executor.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace blockmirror {
namespace p2p {

// 收到的网络消息在这里处理
class MessageVisitor : public boost::static_visitor<> {
 public:
  MessageVisitor(Channel& channel) : _channel(channel) {}

  void operator()(const MsgHeartbeat& msg) const {
    _channel.handleMessage(msg);
  }
  void operator()(const MsgBlock& msg) const { _channel.handleMessage(msg); }
  void operator()(const MsgHello& msg) const { _channel.handleMessage(msg); }
  void operator()(const MsgSyncReq& msg) const { _channel.handleMessage(msg); }
  void operator()(const MsgBroadcastBlock& msg) const {
    _channel.handleMessage(msg);
  }
  void operator()(const MsgGenerateBlock& msg) const {
    _channel.handleMessage(msg);
  }

 private:
  Channel& _channel;
};

// ---------------------------------------------------------------------------------------------------------------------------------------
Channel::Channel(boost::shared_ptr<boost::asio::ip::tcp::socket>& socket,
                 boost::asio::io_context& ioc)
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
  emplaceTimer();  // 安置一个定时器

  sendHello();  //刚连接上，就发hello

  boost::asio::async_read(
      *_socket, /* _binaryBuf */ boost::asio::buffer(_header),
      boost::asio::transfer_exactly(sizeof(MessageHeader)),
      boost::bind(&Channel::handleReadHeader, shared_from_this(),
                  boost::asio::placeholders::error));
}

void Channel::handleReadHeader(const boost::system::error_code& e) {
  if (!e) {
    MessageHeader* header = (MessageHeader*)_header.data();
    if (header->length > MESSAGE_MAX_LENGTH) {
      close();
      return;
    }

    boost::asio::async_read(
        *_socket, _binaryBuf, boost::asio::transfer_exactly(header->length),
        boost::bind(&Channel::handleReadBody, shared_from_this(),
                    boost::asio::placeholders::error));
  }
}

void Channel::handleReadBody(const boost::system::error_code& e) {
  if (!e) {
    Message message;
    blockmirror::serialization::AsioStream stream(_binaryBuf);
    blockmirror::serialization::BinaryIArchive<
        blockmirror::serialization::AsioStream>
        archive(stream);
    archive >> message;
    boost::apply_visitor(MessageVisitor(*this), message);

    // 接着收下一个包
    boost::asio::async_read(
        *_socket, /* _binaryBuf */ boost::asio::buffer(_header),
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
  boost::shared_ptr<boost::asio::streambuf> sb =
      boost::make_shared<boost::asio::streambuf>();
  blockmirror::serialization::AsioStream stream(*sb);
  blockmirror::serialization::BinaryOArchive<
      blockmirror::serialization::AsioStream>
      archive(stream);
  archive << msg;

  auto array = boost::make_shared<boost::array<char, sizeof(MessageHeader)>>();
  MessageHeader* header = (MessageHeader*)array->data();
  header->length = sb->size();

  std::vector<boost::asio::const_buffer> v;
  v.push_back(boost::asio::buffer(*array));  // 先发送头
  v.push_back(sb->data());

  ChannelPtr channel(shared_from_this());
  boost::asio::async_write(
      *_socket, v,
      boost::asio::bind_executor(
          _strand,
          [channel, array, sb](boost::system::error_code ec, std::size_t w) {
            // do nothing
          }));
}

void Channel::handleTimer() {
  if (_connector) {
    send(MsgHeartbeat());  // 每隔10秒向服务端发送心跳包
  } else {
    if (std::time(0) - _current > MSG_HEARTBEAT_TIMEOUT) {
      // 一定时间内没收到客户端的心跳包，认为该连接已僵死，该间隔可以设置得大一点
      close();
      return;
    }
  }

  emplaceTimer();
}

void Channel::emplaceTimer() {
  _timer.expires_from_now(boost::posix_time::seconds(MSG_HEARTBEAT_TICK));
  ChannelWPtr weak(shared_from_this());
  _timer.async_wait([weak](const boost::system::error_code&) {
    auto channel = weak.lock();
    if (!channel) {
      return;
    }

    channel->handleTimer();
  });
}

void Channel::sendBroadcastBlock(const uint64_t& height, const Hash256& hash) {
  MsgBroadcastBlock mbb;
  mbb.height = height;
  mbb.hash = hash;
  send(mbb);
}

void Channel::sendGenerateBlock(blockmirror::chain::BlockPtr block) {
  MsgGenerateBlock mgb;
  mgb._block = block;
  send(mgb);
};

void Channel::sendHello() {
  MsgHello hello;
  hello._node = globalConfig.miner_pubkey;
  hello._addr = _socket->local_endpoint().address().to_string() + ":" +
                boost::lexical_cast<std::string>(globalConfig.p2p_bind);

  blockmirror::chain::Context& c = Server::get().getContext();
  chain::BlockPtr head = c.getHead();
  if (head) {
    hello._hash = head->getHash();
    hello._height = head->getHeight();
  } else {
    hello._hash = {0};
    hello._height = 0;
  }

  send(hello);
}

void Channel::handleMessage(const MsgHello& msg) {
  Hash256 hash = {0};
  uint64_t height = 0;
  blockmirror::chain::Context& c = Server::get().getContext();
  chain::BlockPtr head = c.getHead();
  if (head) {
    hash = head->getHash();
    height = head->getHeight();
  }

  if (height < msg._height) {
    MsgSyncReq req;
    req._start = height + 1;
    req._end = msg._height;
    send(req);
  }
}

void Channel::handleMessage(const MsgSyncReq& msg) {
  blockmirror::chain::Context& c = Server::get().getContext();
  store::BlockStore& store = c.getBlockStore();

  chain::BlockPtr f = c.getHead();
  while (f && f->getHeight() != msg._start) {
    f = store.getBlock(f->getPrevious());
  }

  if (f && f->getHeight() == msg._start) {
    MsgBlock block;
    block._block = f;
    send(block);

    if (msg._start != msg._end) {
      sendHello();
    }
  } else {
    B_ERR("can not find block,height is {}", msg._start);
  }
}

void Channel::handleMessage(const MsgBlock& msg) {
  if (!msg._block) {
    return;
  }

  B_LOG("receive MsgBlock hash:{},height:{}",
        boost::algorithm::hex(std::string(msg._block->getHash().begin(),
                                          msg._block->getHash().end())),
        msg._block->getHeight());

  HandleBlock(msg._block);
}

void Channel::handleMessage(const MsgGenerateBlock& msg) {
  if (!msg._block) {
    return;
  }

  B_LOG("receive MsgGenerateBlock hash:{},height:{}",
        boost::algorithm::hex(std::string(msg._block->getHash().begin(),
                                          msg._block->getHash().end())),
        msg._block->getHeight());

  HandleBlock(msg._block);
}

void Channel::handleMessage(const MsgBroadcastBlock& msg) {
  blockmirror::chain::Context& c = Server::get().getContext();
  store::BlockStore& store = c.getBlockStore();

  if (!store.contains(msg.height, msg.hash)) {
    MsgSyncReq req;
    req._start = msg.height;
    req._end = msg.height;
    send(req);
  }
}

void Channel::HandleBlock(chain::BlockPtr block) {
  blockmirror::chain::Context& c = Server::get().getContext();
  store::BlockStore& store = c.getBlockStore();
  uint64_t height = block->getHeight();
  Hash256 hash = block->getHash();

  if (store.contains(height, hash)) {
    return;
  }
  store.addBlock(block);
  chain::BlockPtr head = c.getHead();
  if (height != (head ? head->getHeight() : 0) + 1) {
    return;
  }

  std::vector<Hash256Ptr> hashes = store.getHash256(height);
  while (!hashes.empty()) {
    for (size_t i = 0; i < hashes.size(); i++) {
      block = store.getBlock(hashes[i]);

      bool success = false;
      if (head) {
        std::vector<chain::BlockPtr> back;
        std::vector<chain::BlockPtr> forward;
        if (store.shouldSwitch(head, block, back, forward)) {
          for (size_t i = 0; i < back.size(); i++) {
            c.rollback();
          }
          for (size_t i = 0; i < forward.size(); i++) {
            c.apply(forward[i]);
          }
          success = true;
        }
      } else {
        c.apply(block);
        success = true;
      }

      if (success) {
        store::MongoStore::get().save(block, &c);
        B_LOG("apply Block hash:{},height:{}",
              boost::algorithm::hex(std::string(block->getHash().begin(),
                                                block->getHash().end())),
              block->getHeight());
        break;
      }
    }

    hashes = store.getHash256(++height);
  }
}

}  // namespace p2p
}  // namespace blockmirror