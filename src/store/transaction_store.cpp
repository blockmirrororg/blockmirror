#include <blockmirror/serialization/binary_iarchive.h>
#include <blockmirror/serialization/binary_oarchive.h>
#include <blockmirror/store/transaction_store.h>

namespace blockmirror {
namespace store {

TransactionStore::TransactionStore() {
  _path = boost::filesystem::initial_path<boost::filesystem::path>();
}
TransactionStore::~TransactionStore() { close(); }

void TransactionStore::load(const boost::filesystem::path& path) {
  if (boost::filesystem::exists(path) &&
      boost::filesystem::is_directory(path)) {
    _path = path;
  }

  std::ifstream stream;
  stream.exceptions(/*std::ifstream::failbit | */std::ifstream::badbit /*|
   std::ifstream::eofbit*/);
  stream.open((_path / "transaction").generic_string(),
              std::ios_base::binary | std::ios_base::in);

  if (stream.is_open()) {
    while (stream.peek() != EOF) {
      serialization::BinaryIArchive<std::ifstream> archive(stream);
      store::TransactionMarkedPtr transm =
          std::make_shared<TransactionMarked>();
      archive >> transm;
      _trs.insert(
          std::make_pair(transm->getTransactionPtr()->getHash(), transm));
    }
  }

  stream.close();
}

void TransactionStore::close() {
  boost::unique_lock<boost::shared_mutex> ulock(_mutex);
  std::ofstream stream;
  stream.exceptions(std::ifstream::failbit | std::ifstream::badbit |
                    std::ifstream::eofbit);
  stream.open((_path / "transaction").generic_string(),
              std::ios_base::binary | std::ios_base::out);
  for (auto n : _trs) {
    serialization::BinaryOArchive<std::ofstream> archive(stream);
    archive << n.second;
  }
  stream.close();
}

bool TransactionStore::exist(const Hash256& h) {
  auto it = _trs.find(h);
  if (it != _trs.end()) {
    return true;
  }
  return false;
}

bool TransactionStore::add(const chain::TransactionPtr& tPtr, uint8_t m) {
  if (nullptr != tPtr) {
    Hash256 h = tPtr->getHash();
    auto it = _trs.find(h);
    if (it == _trs.end()) {
      TransactionMarkedPtr t = std::make_shared<TransactionMarked>(tPtr, m);
      _trs.insert(std::make_pair(h, t));
      return true;
    }
  }
  return false;
}

bool TransactionStore::modify(const Hash256& h, uint8_t m) {
  auto it = _trs.find(h);
  if (it != _trs.end()) {
    it->second->setMark(m);
    return true;
  }
  return false;
}

size_t TransactionStore::remove(const uint64_t& h) {
  size_t count = 0;
  auto it = _trs.begin();
  while (it != _trs.end()) {
    chain::TransactionPtr t = it->second->getTransactionPtr();
    if (t->getExpire() < h) {
      it = _trs.erase(it);
      count++;
    } else {
      it++;
    }
  }
  return count;
}

std::vector<chain::TransactionPtr> TransactionStore::copy() {
  std::vector<chain::TransactionPtr> v;
  std::for_each(_trs.begin(), _trs.end(),
                [&v](std::pair<Hash256, TransactionMarkedPtr> p) {
                  if (p.second->getMark() == UNPACK_STATUS) {
                    v.push_back(p.second->getTransactionPtr());
                  }
                });
  return std::move(v);
}

}  // namespace store
}  // namespace blockmirror