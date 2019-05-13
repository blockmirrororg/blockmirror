#include <blockmirror/store/block_store.h>
#include <blockmirror/store/store.h>
#include <boost/algorithm/hex.hpp>

struct EmptyDeleter {
  void operator()(blockmirror::Hash256 *) {}
};

namespace blockmirror {
namespace store {

BlockStore::BlockStore() : _currentFileIndex(0) {}

BlockStore::~BlockStore() { close(); }

void BlockStore::load(const boost::filesystem::path &path) {
  _path = path;

  if (boost::filesystem::exists(_path / "index")) {
    BinaryReader reader;
    reader.open(_path / "index");
    reader >> _currentFileIndex >> _index;
  }
}

void BlockStore::close() {
  flushBlock(0);
  _ordered.clear();
  _cached.clear();

  BinaryWritter writter;
  writter.open(_path / "index");
  writter << _currentFileIndex << _index;
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

}  // namespace store
}  // namespace blockmirror