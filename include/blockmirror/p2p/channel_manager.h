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
  const std::vector<boost::shared_ptr<Channel>> getChannels();

 private:
  boost::unordered_map<int, boost::weak_ptr<Channel>> _channels;
  boost::atomic_int _channelId;
  boost::asio::detail::mutex _mutex;
};

}  // namespace p2p
}  // namespace blockmirror