#include <blockmirror/store/bps_store.h>
#include <blockmirror/store/store.h>

namespace blockmirror {
namespace store {

BpsStore::BpsStore() : _loaded(false) {}
BpsStore::~BpsStore() {
  if (_loaded) close();
}

void BpsStore::load(const boost::filesystem::path& path) {
  ASSERT(!_loaded);
  _loaded = true;
  _path = path;
  if (boost::filesystem::exists((_path / "bps"))) {
    BinaryReader reader;
    reader.open(_path / "bps");
    reader >> _bps;
    reader >> _changes;
  }
}

void BpsStore::close() {
  ASSERT(_loaded);
  _loaded = false;
  BinaryWritter writter;
  writter.open(_path / "bps");
  writter << _bps;
  writter << _changes;
}

bool BpsStore::contains(const Pubkey& key) {
  return std::find(_bps.begin(), _bps.end(), key) != _bps.end();
}

int BpsStore::find(const Pubkey& key) {
  auto i = std::find(_bps.begin(), _bps.end(), key);
  if (i != _bps.end()) {
    return (int)(i - _bps.begin());
  }
  return -1;
}

bool BpsStore::add(const Pubkey& key) {
  if (contains(key)) return false;
  _bps.emplace_back(key);
  return true;
}

bool BpsStore::remove(const Pubkey& key) {
  auto i = std::find(_bps.begin(), _bps.end(), key);
  if (i != _bps.end()) {
    _bps.erase(i);
    return true;
  }
  return false;
}

int BpsStore::getBPDelay(const Pubkey& key, uint64_t now) {
  int off = find(key);
  if (off < 0) return -1;  // 不是BP
  if (_changes.empty()) {
    // 第一个块
    // 执行第一个块应该 pushBpChange(0, block.timestamp)
    // 撤回第一个块应该 popBpChange()
    return 0;
  }
  // 当前出块序号
  auto nowSlots = getSlotByTime(now);

  // 确保在当前序号以后
  if (off < nowSlots) {
    off += _bps.size();
  } else if (off == nowSlots) {
    if (now % BLOCK_PER_MS > 0) {
      off += _bps.size();
    }
  }

  return ((off - nowSlots) * BLOCK_PER_MS) - now % BLOCK_PER_MS;
}

void BpsStore::pushBpChange(uint64_t timestamp) {
  ASSERT(isAlignedTime(timestamp));
  uint64_t idx = 0;
  if (!_changes.empty()) {
    idx = (timestamp - _changes.back().timestamp) / BLOCK_PER_MS +
          _changes.back().index;
  }
  _changes.emplace_back(idx, timestamp);
  B_LOG("push bp change: {} {}", idx, timestamp);
}
void BpsStore::popBpChange() {
  ASSERT(!_changes.empty());
  B_LOG("push bp change: {} {}", _changes.back().index,
        _changes.back().timestamp);
  _changes.pop_back();
}

}  // namespace store
}  // namespace blockmirror