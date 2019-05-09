#include <blockmirror/store/store.h>
#include <blockmirror/store/transaction_store.h>

namespace blockmirror {
namespace store {

void TransactionStore::load(const boost::filesystem::path &path) {
  _path = path;

  if (boost::filesystem::exists(_path / "transaction")) {
    BinaryReader reader;
    reader.open(_path / "transaction");
    reader >> *this;
  }
}

void TransactionStore::close() {
  BinaryWritter writter;
  writter.open(_path / "transaction");
  writter << *this;
}

bool TransactionStore::contains(const Hash256Ptr &h) {
  boost::shared_lock<boost::shared_mutex> lock(_mutex);
  return _container.get<tagHash>().find(h) != _container.end();
}

bool TransactionStore::add(const chain::TransactionSignedPtr &trx,
                           uint64_t height) {
  boost::unique_lock<boost::shared_mutex> lock(_mutex);
  auto r = _container.emplace(trx, height);
  if (!r.second) {
    _container.modify(r.first, [height](TransactionItem &v) {
      v.height = height;
    });
  }
  return r.second;
}

// lvjl
std::vector<chain::TransactionSignedPtr> TransactionStore::popUnpacked() {
  std::vector<chain::TransactionSignedPtr> r;
  boost::unique_lock<boost::shared_mutex> lock(_mutex);
  auto &idx = _container.get<tagHeight>();
  for (auto i = idx.begin(); i != idx.end();) {
    if (i->height > 0) {
      return std::move(r);
    } else {
      r.push_back(i->transaction);
      i = idx.erase(i);
    }
  }
  return std::move(r);
}

void TransactionStore::removeExpired(uint64_t height) {
  boost::unique_lock<boost::shared_mutex> lock(_mutex);
  auto &idx = _container.get<tagExpire>();
  auto i = idx.begin();
  while (i != idx.end()) {
    if (i->expire() > height) return;
    i = idx.erase(i);
  }
}

}  // namespace store
}  // namespace blockmirror