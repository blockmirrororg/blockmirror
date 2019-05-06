
#include <blockmirror/common.h>
#include <blockmirror/serialization/access.h>

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
  uint16_t magic;
  uint16_t length;
};
static const uint64_t MESSAGE_MAX_LENGTH = 65535;

BOOST_STATIC_ASSERT(sizeof(MessageHeader) == 4);

using Message = boost::variant<MsgHeartbeat, MsgBroadcastBlock>;

}  // namespace p2p
}  // namespace blockmirror