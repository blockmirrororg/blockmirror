#include <blockmirror/store/account_store.h>
#include <blockmirror/store/store.h>

namespace blockmirror {
namespace store {

AccountStore::AccountStore() : _loaded(false) {
  _path = boost::filesystem::initial_path<boost::filesystem::path>();
}
AccountStore::~AccountStore() {
  if (_loaded) close();
}

void AccountStore::load(const boost::filesystem::path &path) {
  ASSERT(!_loaded);
  _loaded = true;
  _path = path;
  if (boost::filesystem::exists(_path / "account")) {
    BinaryReader reader;
    reader.open(_path / "account");
    reader >> _accounts;
  }
}

void AccountStore::close() {
  ASSERT(_loaded);
  _loaded = false;
  BinaryWritter writter;
  writter.open(_path / "account");
  writter << _accounts;
}

uint64_t AccountStore::query(const Pubkey &pubkey) {
  boost::shared_lock<boost::shared_mutex> lock(_mutex);
  auto i = _accounts.find(pubkey);
  if (i == _accounts.end()) return 0;
  return i->second;
}

bool AccountStore::add(const Pubkey &pubkey, int64_t amount) {
  boost::unique_lock<boost::shared_mutex> lock(_mutex);
  auto finded = _accounts.find(pubkey);
  if (amount > 0) {
    if (finded == _accounts.end()) {
      auto r = _accounts.insert(std::make_pair(pubkey, amount));
      return r.second;
    } else {
      finded->second += amount;
      return true;
    }
  } else {
    if (finded == _accounts.end()) {
      return false;
    }
    if (finded->second < -amount) {
      return false;
    }
    finded->second += amount;
    return true;
  }
}

bool AccountStore::transfer(const Pubkey &from, const Pubkey &to,
                            uint64_t amount) {
  boost::upgrade_lock<boost::shared_mutex> lock(_mutex);
  auto fromIter = _accounts.find(from);
  auto toIter = _accounts.find(to);
  if (fromIter == _accounts.end() || fromIter->second < amount) {
    return false;
  }
  boost::upgrade_to_unique_lock<boost::shared_mutex> upgrade(lock);
  if (toIter == _accounts.end()) {
    auto insert = _accounts.insert(std::make_pair(to, amount));
    if (!insert.second) return false;
  } else {
    toIter->second += amount;
  }
  fromIter->second -= amount;
  return true;
}

}  // namespace store
}  // namespace blockmirror