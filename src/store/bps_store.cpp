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

int BpsStore::calcBPOffset(const Pubkey& key, uint64_t now) {
  int off = find(key);
  if (off < 0) return -1;
  if (_changes.empty()) {
    // 执行第一个块应该 pushBpChange(0, block.timestamp)
    // 撤回第一个块应该 popBpChange()
    return 0;
  }
  // 一圈需要多久
  const auto loopMillSeconds = _bps.size() * BLOCK_PER_MS;

  auto& records = _changes.back();
  // 现在到最后一个记录的总槽数
  auto slots = (now / BLOCK_PER_MS) - (records.timestamp / BLOCK_PER_MS);
  // 现在到第一个块的总槽数
  slots += records.index;
  // 现在应该选择的BP
  auto curLoop = (slots / _bps.size()) * _bps.size();
  auto result =
      ((off + curLoop) - records.index) * BLOCK_PER_MS + records.timestamp;
  while (result < now) {
    result += loopMillSeconds;
  }
  return result - now;
}

void BpsStore::pushBpChange(uint64_t idx, uint64_t timestamp) {
  _changes.emplace_back(idx, timestamp);
}
void BpsStore::popBpChange() { _changes.pop_back(); }

}  // namespace store
}  // namespace blockmirror