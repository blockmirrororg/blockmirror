#include <blockmirror/store/bps_store.h>
#include <blockmirror/store/store.h>

namespace blockmirror {
namespace store {

BpsStore::BpsStore() : _loaded(false) {
  _path = boost::filesystem::initial_path<boost::filesystem::path>();
}
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
  }
}

void BpsStore::close() {
  ASSERT(_loaded);
  _loaded = false;
  BinaryWritter writter;
  writter.open(_path / "bps");
  writter << _bps;
}

bool BpsStore::contains(const Pubkey& key) {
  boost::shared_lock<boost::shared_mutex> lock(_mutex);
  return _bps.find(key) != _bps.end();
}

bool BpsStore::add(const Pubkey& key) {
  boost::unique_lock<boost::shared_mutex> lock(_mutex);
  auto r = _bps.insert(key);
  return r.second;
}

bool BpsStore::remove(const Pubkey& key) {
  boost::unique_lock<boost::shared_mutex> lock(_mutex);
  return _bps.erase(key) > 0;
}

}  // namespace store
}  // namespace blockmirror