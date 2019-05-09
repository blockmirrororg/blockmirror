#pragma once

#include "channel.h"
#include <boost/weak_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/atomic.hpp>

namespace blockmirror {
namespace p2p {

class ChannelManager {
 public:
  ChannelManager();

  static ChannelManager &get();
  void add_channel(boost::shared_ptr<Channel> &channel);
  void remove_channel(int channel_id);
  boost::shared_ptr<Channel> find_channel(int channel_id);

 private:
  boost::unordered_map<int, boost::weak_ptr<Channel> > channels_;
  boost::atomic_int channel_id_;
  boost::asio::detail::mutex mutex_;
};

}  // namespace p2p
}  // namespace blockmirror