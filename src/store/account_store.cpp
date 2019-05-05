#include <blockmirror/serialization/binary_iarchive.h>
#include <blockmirror/serialization/binary_oarchive.h>
#include <blockmirror/store/account_store.h>

namespace blockmirror {
namespace store {

AccountStore::AccountStore() {
  _path = boost::filesystem::initial_path<boost::filesystem::path>();
}
AccountStore::~AccountStore() { close(); }

void AccountStore::load(const boost::filesystem::path &path) {
  if (boost::filesystem::exists(path) &&
      boost::filesystem::is_directory(path)) {
    _path = path;
  }

  std::ifstream stream;
  stream.exceptions(/*std::ifstream::failbit | */std::ifstream::badbit /*|
   std::ifstream::eofbit*/);
  stream.open((_path / "account").generic_string(),
              std::ios_base::binary | std::ios_base::in);

  if (stream.is_open()) {
    while (stream.peek() != EOF) {
      serialization::BinaryIArchive<std::ifstream> archive(stream);
      store::AccountPtr account = std::make_shared<Account>();
      archive >> account;
      _accounts.insert(
          std::make_pair(account->getPubkey(), account->getBalance()));
    }
  }

  stream.close();
}

void AccountStore::close() {
  boost::unique_lock<boost::shared_mutex> ulock(_mutex);
  std::ofstream stream;
  stream.exceptions(std::ifstream::failbit | std::ifstream::badbit |
                    std::ifstream::eofbit);
  stream.open((_path / "account").generic_string(),
              std::ios_base::binary | std::ios_base::out);
  for (auto n : _accounts) {
    serialization::BinaryOArchive<std::ofstream> archive(stream);
    store::AccountPtr account = std::make_shared<Account>(n.first, n.second);
    archive << account;
  }
  stream.close();
}

uint64_t AccountStore::query(const Pubkey &pubkey) {
  boost::shared_lock<boost::shared_mutex> lock(_mutex);
  auto i = _accounts.find(pubkey);
  if (i == _accounts.end()) return 0;
  return i->second;
}

bool AccountStore::add(const Pubkey &pubkey, uint64_t amount) {
  auto it = _accounts.find(pubkey);
  if (it == _accounts.end()) {
    _accounts.insert(std::make_pair(pubkey, amount));
    return true;
  }
  return false;
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