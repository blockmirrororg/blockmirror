#include <blockmirror/store/store.h>
#include <blockmirror/store/transaction_store.h>

namespace blockmirror {
namespace store {

void TransactionStore::load(const boost::filesystem::path &path) {
  ASSERT(!_loaded);
  _loaded = true;
  _path = path;

  if (boost::filesystem::exists(_path / "transaction")) {
    BinaryReader reader;
    reader.open(_path / "transaction");
    reader >> *this;
  }
}

void TransactionStore::close() {
  ASSERT(_loaded);
  _loaded = false;
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
    _container.modify(r.first,
                      [height](TransactionItem &v) { v.height = height; });
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

bool TransactionStore::remove(const chain::TransactionSignedPtr &trx) {
  boost::unique_lock<boost::shared_mutex> lock(_mutex);
  auto &idx = _container.get<tagHash>();
  return idx.erase(trx->getHashPtr()) > 0;
}

chain::TransactionSignedPtr TransactionStore::getTransaction(
    const Hash256Ptr &h) {

  boost::shared_lock<boost::shared_mutex> lock(_mutex);
  auto pos = _container.get<tagHash>().find(h);
  if (pos == _container.end()) {
    return std::shared_ptr<chain::TransactionSigned>();
  } else {
    return pos->transaction;
  }
}

}  // namespace store
}  // namespace blockmirror