#pragma once

#include <boost/atomic.hpp>
#include <boost/unordered_map.hpp>
#include <boost/weak_ptr.hpp>
#include "channel.h"

namespace blockmirror {
namespace p2p {

class ChannelManager {
 public:
  ChannelManager();

  static ChannelManager& get() {
    static ChannelManager channel_mgr;
    return channel_mgr;
  }

  void addChannel(boost::shared_ptr<Channel> channel);
  void removeChannel(int id);
  boost::shared_ptr<Channel> findChannel(int id);

  std::pair<uint64_t, uint64_t> searchBlocks(uint64_t start, uint64_t end);
  void syncBlocks(uint64_t h, blockmirror::chain::BlockPtr b);
  const std::vector<boost::shared_ptr<Channel>> getChannels();
  bool blocksContain(const uint64_t& height);

 private:
  boost::unordered_map<int, boost::weak_ptr<Channel>> _channels;
  boost::atomic_int _channelId;
  boost::asio::detail::mutex _mutex;

  std::map<uint64_t, blockmirror::chain::BlockPtr> _blocks;
};

}  // namespace p2p
}  // namespace blockmirror