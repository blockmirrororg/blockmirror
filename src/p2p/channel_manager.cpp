
#include <blockmirror/p2p/channel_manager.h>

namespace blockmirror {
namespace p2p {

ChannelManager::ChannelManager() : channel_id_(-1) {}

ChannelManager &ChannelManager::get() {
  static ChannelManager channel_mgr;
  return channel_mgr;
}

void ChannelManager::add_channel(boost::shared_ptr<Channel> &channel) {
	channel->id(++channel_id_);

  boost::asio::detail::mutex::scoped_lock lock(mutex_);
  channels_.insert(std::pair<int, boost::weak_ptr<Channel> >(channel->id(), channel));
}

boost::shared_ptr<Channel> ChannelManager::find_channel(int channel_id)
{
	boost::asio::detail::mutex::scoped_lock lock(mutex_);
	boost::unordered_map<int, boost::weak_ptr<Channel> >::iterator pos = channels_.find(channel_id);
	if (pos != channels_.end())
		return pos->second.lock();
	else
		return boost::shared_ptr<Channel>();
}

}  // namespace p2p
}  // namespace blockmirror