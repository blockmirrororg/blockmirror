
#include <blockmirror/p2p/channel_manager.h>

namespace blockmirror {
namespace p2p {

ChannelManager::ChannelManager() : _channelId(-1) {}

void ChannelManager::addChannel(boost::shared_ptr<Channel> channel) {
  channel->id(++_channelId);

  boost::unique_lock<boost::shared_mutex> lock(_mutex);
  _channels.insert(
      std::pair<int, boost::weak_ptr<Channel>>(channel->id(), channel));
}

boost::shared_ptr<Channel> ChannelManager::findChannel(int id) {
  boost::shared_lock<boost::shared_mutex> lock(_mutex);
  auto pos = _channels.find(id);
  if (pos != _channels.end())
    return pos->second.lock();
  else
    return nullptr;
}

void ChannelManager::removeChannel(int id) {
  boost::unique_lock<boost::shared_mutex> lock(_mutex);
  auto pos = _channels.find(id);
  if (pos != _channels.end()) {
    _channels.erase(pos);
  }
}

const std::vector<boost::shared_ptr<Channel>> ChannelManager::getChannels() {
  boost::shared_lock<boost::shared_mutex> lock(_mutex);
  std::vector<boost::shared_ptr<Channel>> v;
  for (auto i : _channels) {
    boost::shared_ptr<Channel> channel = i.second.lock();
    if (channel) {
      v.push_back(channel);
    }
  }
  return std::move(v);
}

boost::shared_mutex& ChannelManager::getChannelMutex() { return _channelMutex; }

bool ChannelManager::channelsSyncDone() {
  boost::shared_lock<boost::shared_mutex> lock(_mutex);
  for (auto ch : _channels) {
    boost::shared_ptr<Channel> channel = ch.second.lock();
    if (channel && channel->getStatus().isSync()) {
      B_LOG("channels are synchronizing blocks");
      return false;
    }
  }
  return true;
}

}  // namespace p2p
}  // namespace blockmirror