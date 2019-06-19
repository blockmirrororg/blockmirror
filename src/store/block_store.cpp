#include <blockmirror/store/block_store.h>
#include <blockmirror/store/store.h>
#include <boost/algorithm/hex.hpp>

struct EmptyDeleter {
  void operator()(blockmirror::Hash256 *) {}
};

namespace blockmirror {
namespace store {

BlockStore::BlockStore() : _currentFileIndex(0), _loaded(false) {}

BlockStore::~BlockStore() {
  if (_loaded) close();
}

void BlockStore::load(const boost::filesystem::path &path) {
  ASSERT(!_loaded);
  _loaded = true;
  _path = path;

  if (boost::filesystem::exists(_path / "index")) {
    BinaryReader reader;
    reader.open(_path / "index");
    reader >> _currentFileIndex >> _index >> _received;
  }
}

void BlockStore::close() {
  ASSERT(_loaded);
  _loaded = false;
  flushBlock(0);
  _ordered.clear();
  _cached.clear();

  BinaryWritter writter;
  writter.open(_path / "index");
  writter << _currentFileIndex << _index << _received;
}

chain::BlockPtr BlockStore::_loadBlock(uint64_t index) {
  uint32_t file = getFile(index);
  uint32_t offset = getOffset(index);

  BinaryReader reader;
  reader.open(_path / (boost::lexical_cast<std::string>(file) + ".block"));
  reader.seekg(offset);
  chain::BlockPtr block = std::make_shared<chain::Block>();
  reader >> block;
  return block;
}

uint64_t BlockStore::_saveBlock(chain::BlockPtr block) {
  boost::unique_lock<boost::mutex> lock(_fileMutex);

  BinaryWritter writter;
  writter.open(
      _path / (boost::lexical_cast<std::string>(_currentFileIndex) + ".block"),
      std::ios_base::binary | std::ios_base::out | std::ios_base::app);
#if defined(WIN32) || defined(_WIN32)
  writter.seekEnd();
#endif
  uint32_t file = _currentFileIndex;
  uint32_t offset = (uint32_t)writter.tellp();
  writter << block;
  if (writter.tellp() >= BLOCKSTORE_MAX_FILE) {
    _currentFileIndex++;
  }
  writter.close();
  return makeIndex(offset, file);
}

bool BlockStore::contains(const Hash256Ptr &hash) {
  boost::shared_lock<boost::shared_mutex> lock(_mutex);
  if (_cached.find(hash) != _cached.end()) {
    return true;
  }
  return _index.find(hash) != _index.end();
}

bool BlockStore::contains(const Hash256 &hash) {
  Hash256Ptr hashPtr(const_cast<Hash256 *>(&hash), EmptyDeleter());
  return contains(hashPtr);
}

chain::BlockPtr BlockStore::getBlock(const Hash256 &hash) {
  Hash256Ptr hashPtr(const_cast<Hash256 *>(&hash), EmptyDeleter());
  return getBlock(hashPtr);
}
chain::BlockPtr BlockStore::getBlock(const Hash256Ptr &hash) {
  uint64_t index;
  {
    boost::shared_lock<boost::shared_mutex> lock(_mutex);
    // 缓存
    auto cacheIter = _cached.find(hash);
    if (cacheIter != _cached.end()) {
      return cacheIter->second;
    }
    // 文件
    auto indexIter = _index.find(hash);
    if (indexIter == _index.end()) {
      return chain::BlockPtr();
    }
    index = indexIter->second;
  }
  return _loadBlock(index);
}

bool BlockStore::addBlock(chain::BlockPtr &block) {
  boost::unique_lock<boost::shared_mutex> lock(_mutex);
  auto orderedR = _ordered.insert(block);
  if (!orderedR.second) return false;
  VERIFY(_cached.insert(std::make_pair(block->getHashPtr(), block)).second);

  auto it = _received.find(block->getHeight());
  if (it == _received.end()) {
    _received.insert(std::make_pair(
        block->getHeight(), std::vector<Hash256Ptr>{block->getHashPtr()}));
  } else {
    if (std::find(it->second.begin(), it->second.end(), block->getHashPtr()) ==
        it->second.end()) {
      it->second.push_back(block->getHashPtr());
    }
  }

  return true;
}

void BlockStore::flushBlock(size_t storeLimit) {
  std::vector<chain::BlockPtr> saving;
  {
    boost::shared_lock<boost::shared_mutex> lock(_mutex);
    int count = (int)_ordered.size() - (int)storeLimit;
    auto iter = _ordered.begin();
    for (int i = 0; i < count; i++) {
      saving.push_back(*iter);
      ++iter;
    }
  }
  if (saving.empty()) return;
  std::vector<uint64_t> indexes;
  for (size_t i = 0; i < saving.size(); i++) {
    indexes.push_back(_saveBlock(saving[i]));
  }
  {
    boost::unique_lock<boost::shared_mutex> lock(_mutex);
    for (size_t i = 0; i < saving.size(); i++) {
      auto &blk = saving[i];
      // 从 ordered 和 cached 删除这些区块
      VERIFY(_ordered.erase(blk) == 1);
      VERIFY(_cached.erase(blk->getHashPtr()) == 1);
      // 添加到 index 中
      VERIFY(
          _index.insert(std::make_pair(blk->getHashPtr(), indexes[i])).second);
    }
  }
}

bool BlockStore::shouldSwitch(const chain::BlockPtr &head,
                              const chain::BlockPtr &fork,
                              std::vector<chain::BlockPtr> &back,
                              std::vector<chain::BlockPtr> &forward) {
  back.clear();
  forward.clear();
  if (fork->getHeight() <= head->getHeight()) {
    return false;
  }
  // 1. 先将fork退回到同一个高度
  chain::BlockPtr f = fork;
  while (f->getHeight() > head->getHeight()) {
    forward.push_back(f);
    f = getBlock(f->getPrevious());
  }
  // 2. 两边开始一直退到同一个区块
  chain::BlockPtr h = head;
  while (f->getHash() != h->getHash()) {
    back.push_back(h);
    if (back.size() >= BLOCK_MAX_ROLLBACK) {
      return false;
    }
    forward.push_back(f);
    f = getBlock(f->getPrevious());
    h = getBlock(h->getPrevious());
  }
  return true;
}

const std::vector<Hash256Ptr> BlockStore::getHash256(const uint64_t &height) {
  boost::unique_lock<boost::shared_mutex> lock(_mutex);
  std::vector<Hash256Ptr> v;
  auto it = _received.find(height);
  if (it != _received.end()) {
    v.assign(it->second.begin(), it->second.end());
  }
  return std::move(v);
}

bool BlockStore::contains(const uint64_t &height, const Hash256 &hash) {
  boost::shared_lock<boost::shared_mutex> lock(_mutex);
  auto it = _received.find(height);
  if (it != _received.end()) {
    for (auto i : it->second) {
      if (*i == hash) {
        return true;
      }
    }
  }
  return false;
}

}  // namespace store
}  // namespace blockmirror