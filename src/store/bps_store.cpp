#include <blockmirror/serialization/binary_iarchive.h>
#include <blockmirror/serialization/binary_oarchive.h>
#include <blockmirror/store/bps_store.h>

namespace blockmirror {
namespace store {

BpsStore::BpsStore() {
  _path = boost::filesystem::initial_path<boost::filesystem::path>();
}
BpsStore::~BpsStore() { close(); }

void BpsStore::load(const boost::filesystem::path& path) {
  if (boost::filesystem::exists(path) &&
      boost::filesystem::is_directory(path)) {
    _path = path;
  }

  std::ifstream stream;
  stream.exceptions(/*std::ifstream::failbit |*/ std::ifstream::badbit /*|
                    //std::ifstream::eofbit*/);
  stream.open((_path / "bps").generic_string(),
              std::ios_base::binary | std::ios_base::in);

  if (stream.is_open()) {
    while (stream.peek() != EOF) {
      serialization::BinaryIArchive<std::ifstream> archive(stream);
      store::BPJoinPtr join = std::make_shared<chain::scri::BPJoin>();
      archive >> join;
      Pubkey key = join->getBP();
      _bps.insert(std::make_pair(key, join));
    }
  }

  stream.close();
}

void BpsStore::close() {
  boost::unique_lock<boost::shared_mutex> ulock(_mutex);
  std::ofstream stream;
  stream.exceptions(std::ifstream::failbit | std::ifstream::badbit |
                    std::ifstream::eofbit);
  stream.open((_path / "bps").generic_string(),
              std::ios_base::binary | std::ios_base::out);
  for (auto n : _bps) {
    serialization::BinaryOArchive<std::ofstream> archive(stream);
    archive << n.second;
  }
  stream.close();
}

store::BPJoinPtr BpsStore::query(const Pubkey& key) {
  boost::shared_lock<boost::shared_mutex> slock(_mutex);
  auto it = _bps.find(key);
  if (it == _bps.end()) {
    return nullptr;
  }
  return it->second;
}

bool BpsStore::add(const store::BPJoinPtr& joinPtr) {
  boost::unique_lock<boost::shared_mutex> ulock(_mutex);
  Pubkey key = joinPtr->getBP();
  auto it = _bps.find(key);
  if (it != _bps.end()) {
    return false;
  }
  _bps.insert(std::make_pair(key, joinPtr));
  return true;
}

bool BpsStore::reduce(const store::BPExitPtr& exitPtr) {
  boost::unique_lock<boost::shared_mutex> ulock(_mutex);
  Pubkey key = exitPtr->getBP();
  auto it = _bps.find(key);
  if (it != _bps.end()) {
    _bps.erase(key);
    return true;
  }
  return false;
}

}  // namespace store
}  // namespace blockmirror