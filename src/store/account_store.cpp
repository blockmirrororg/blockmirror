#include <blockmirror/serialization/binary_iarchive.h>
#include <blockmirror/serialization/binary_oarchive.h>
#include <blockmirror/store/account_store.h>

namespace blockmirror {
namespace store {

AccountStore::AccountStore() {
  
}
AccountStore::~AccountStore() { close(); }

void AccountStore::load(const boost::filesystem::path &path) {
  // 从文件中加载 _accounts
}

void AccountStore::close() {
  // 保存到文件 _accounts
}
uint64_t AccountStore::query(const Pubkey &pubkey) {
  boost::shared_lock<boost::shared_mutex> lock(_mutex);
  auto i = _accounts.find(pubkey);
  if (i == _accounts.end()) return 0;
  return i->second;
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