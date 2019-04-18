#include <blockmirror/chain/block.h>
#include <blockmirror/serialization/binary_oarchive.h>
#include <blockmirror/serialization/hash_ostream.h>

namespace blockmirror {
namespace chain {

const Hash256 &BlockHeader::getHash() const {
  if (_hash) {
    return *_hash;
  }
  serialization::HashOStream oss;
  serialization::BinaryOArchive<decltype(oss)> oa(oss);
  oa << *this;
  _hash = std::make_shared<Hash256>();
  oss.getHash(*_hash);
  return *_hash;
}

void BlockHeader::setPrevious(const BlockHeader &parent) {
  _hash.reset();
  previous = parent.getHash();
  height = parent.getHeight() + 1;
  timestamp = now_ms_since_1970();
}

void BlockHeader::setGenesis() {
  _hash.reset();
  timestamp = now_ms_since_1970();
  height = 1;
  previous.fill(0);
}

void BlockHeaderSigned::sign(const Privkey &priv,
                             const crypto::ECCContext &ecc) {
  ecc.computePub(priv, producer);
  ecc.sign(priv, getHash(), signature);
}

bool BlockHeaderSigned::verify(const crypto::ECCContext &ecc) const {
  return ecc.verify(producer, getHash(), signature);
}

void Block::addDataBP(DataBPPtr &data) {
  datas.push_back(data);
}

void Block::addTransaction(TransactionSignedPtr &trx) {
  transactions.push_back(trx);
}
void Block::setCoinbase(const Pubkey &target, uint64_t amount) {
  coinbase = std::make_shared<Transaction>(script::Transfer(target, amount));
}
void Block::finalize(const Privkey &priv, const crypto::ECCContext &ecc) {
  merkleTransaction.fill(0);
  merkleData.fill(0);
  // FIXME: 计算默克数
  sign(priv, ecc);
}

}  // namespace chain
}  // namespace blockmirror
