
#include <blockmirror/p2p/channel_manager.h>

namespace blockmirror {
namespace p2p {

ChannelManager::ChannelManager() : _channelId(-1) {}

void ChannelManager::addChannel(boost::shared_ptr<Channel> channel) {
  channel->id(++_channelId);

  boost::asio::detail::mutex::scoped_lock lock(_mutex);
  _channels.insert(
      std::pair<int, boost::weak_ptr<Channel> >(channel->id(), channel));
}

boost::shared_ptr<Channel> ChannelManager::findChannel(int id) {
  boost::asio::detail::mutex::scoped_lock lock(_mutex);
  auto pos = _channels.find(id);
  if (pos != _channels.end())
    return pos->second.lock();
  else
    return nullptr;
}

void ChannelManager::removeChannel(int id)
{
  boost::asio::detail::mutex::scoped_lock lock(_mutex);
  auto pos = _channels.find(id);
  if (pos != _channels.end())
  {
    _channels.erase(pos);
  }
}

}  // namespace p2p
}  // namespace blockmirror