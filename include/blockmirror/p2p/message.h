
#include <blockmirror/common.h>
#include <blockmirror/serialization/access.h>
#include <blockmirror/chain/block.h>

namespace blockmirror {
namespace p2p {

// 心跳消息
class MsgHeartbeat {
  friend class blockmirror::serialization::access;
  template <typename Archive>
  void serialize(Archive &) {}
};
static const uint64_t MSG_HEARTBEAT_TICK = 1000 * 7;  // 每10秒一个心跳
static const uint64_t MSG_HEARTBEAT_TIMEOUT = 1000 * 15;  // 15秒没有消息则超时

// 刚产生一个区块
class MsgGenerateBlock {
  friend class blockmirror::serialization::access;
  template<typename Archive>
  void serialize(Archive &ar)
  {
    ar &BOOST_SERIALIZATION_NVP(_block);
  }

  blockmirror::chain::BlockPtr _block;
};

//握手
class MsgHello {
  friend class boost::serialization::access;
  template <typename Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(node) & BOOST_SERIALIZATION_NVP(addr) &
        BOOST_SERIALIZATION_NVP(head) & BOOST_SERIALIZATION_NVP(height);
  }
  Pubkey node;       // 节点公钥
  std::string addr;  // p2p地址
  Hash256 head;      // head 哈希
  uint64_t height;   // head 高度
};

//请求区块
class MsgSyncReq {
  friend class boost::serialization::access;
  template <typename Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(start) & BOOST_SERIALIZATION_NVP(end);
  }
  uint64_t start;
  uint64_t end;
};

// 广播区块消息 当我获得了一个完整的区块时 向所有连接发送此消息
// 1. 刚出完块
// 2. 刚执行完一个网络上收到的块
class MsgBroadcastBlock {
  friend class blockmirror::serialization::access;
  template <typename Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(height) & BOOST_SERIALIZATION_NVP(hash);
  }

  uint64_t height;  // 所广播的高度
  Hash256 hash;     // 所广播的HASH
};

struct MessageHeader {
  friend class blockmirror::serialization::access;
  template <typename Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(magic) & BOOST_SERIALIZATION_NVP(length);
  }

  uint16_t magic = 0;
  uint16_t length = 0;
};
static const uint64_t MESSAGE_MAX_LENGTH = 65535;

BOOST_STATIC_ASSERT(sizeof(MessageHeader) == 4);

using Message = boost::variant<MsgHeartbeat, MsgBroadcastBlock, MsgGenerateBlock>;

}  // namespace p2p
}  // namespace blockmirror