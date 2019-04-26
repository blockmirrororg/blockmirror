
#include <blockmirror/common.h>
#include <blockmirror/serialization/access.h>

namespace blockmirror {
namespace p2p {

// 心跳消息，心跳没有什么实质上的内容
class MsgHeartbeat {
  friend class blockmirror::serialization::access;
  template <typename Archive>
  void serialize(Archive &) {
  }
};

// 广播区块消息 当我获得了一个完整的区块时 向所有连接发送此消息
class MsgBroadcastBlock {
  friend class blockmirror::serialization::access;
  template <typename Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(height) & BOOST_SERIALIZATION_NVP(hash);
  }

  uint64_t height; // 所广播的高度
  Hash256 hash; // 所广播的HASH
};

using Message = boost::variant<MsgHeartbeat, MsgBroadcastBlock>;

}  // namespace p2p
}  // namespace blockmirror