
#include <blockmirror/p2p/channel_manager.h>
#include <blockmirror/server.h>

namespace blockmirror {
namespace p2p {

ChannelManager::ChannelManager() : _channelId(-1) {}

void ChannelManager::addChannel(boost::shared_ptr<Channel> channel) {
  channel->id(++_channelId);

  boost::asio::detail::mutex::scoped_lock lock(_mutex);
  _channels.insert(
      std::pair<int, boost::weak_ptr<Channel>>(channel->id(), channel));
}

boost::shared_ptr<Channel> ChannelManager::findChannel(int id) {
  boost::asio::detail::mutex::scoped_lock lock(_mutex);
  auto pos = _channels.find(id);
  if (pos != _channels.end())
    return pos->second.lock();
  else
    return nullptr;
}

void ChannelManager::removeChannel(int id) {
  boost::asio::detail::mutex::scoped_lock lock(_mutex);
  auto pos = _channels.find(id);
  if (pos != _channels.end()) {
    _channels.erase(pos);
  }
}

std::pair<uint64_t, uint64_t> ChannelManager::searchBlocks(uint64_t start,
                                                           uint64_t end) {
  boost::asio::detail::mutex::scoped_lock lock(_mutex);
  uint64_t s = start + 1;
  while (true) {
    auto it = _blocks.find(s);
    if (it == _blocks.end()) {
      break;
    }
    s++;
    if (s >= end) {
      break;
    }
  }

  uint64_t e = end;
  auto it = _blocks.find(e);
  if (it != _blocks.end()) {
    for (; e > s; e--) {
      if (!blocksContain(e)) {
        break;
      }
    }
  }

  return std::pair<uint64_t, uint64_t>(s, e);
}

void ChannelManager::syncBlocks(uint64_t h, blockmirror::chain::BlockPtr b) {
  boost::asio::detail::mutex::scoped_lock lock(_mutex);

  _blocks.insert(std::pair<uint64_t, blockmirror::chain::BlockPtr>(h, b));

  blockmirror::chain::Context& c = Server::get().getContext();
  Hash256 hash = {0};
  uint64_t height = 0;
  while (true) {
    chain::BlockPtr head = c.getHead();
    if (head) {
      hash = head->getHash();
      height = head->getHeight();
    }
    auto it = _blocks.begin();
    if (it->first == height + 1 && it->second->getPrevious() == hash) {
      bool r = c.apply(it->second);
      if (r) {
        store::BlockStore& store = c.getBlockStore();
        store.addBlock(it->second);
        const std::vector<boost::shared_ptr<Channel>> channels = getChannels();
        for (auto i : channels) {
          i->sendBroadcastBlock(it->first, it->second->getHash());
        }
      }
      _blocks.erase(it);
      if (!r) {
        break;
      }
    } else if (it->first == height + 1 && it->second->getPrevious() != hash) {
      _blocks.erase(it);
      break;
    } else {
      break;
    }
  }
}

const std::vector<boost::shared_ptr<Channel>> ChannelManager::getChannels() {
  std::vector<boost::shared_ptr<Channel>> v;
  for (auto i : _channels) {
    if (i.second.lock())
    {
      v.push_back(i.second.lock());
    }
  }
  return std::move(v);
}

bool ChannelManager::blocksContain(const uint64_t& height) {
  return _blocks.find(height) != _blocks.end();
}

}  // namespace p2p
}  // namespace blockmirror