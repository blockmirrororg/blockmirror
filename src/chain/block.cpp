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
  timestamp = alignTimestamp(now_ms_since_1970());
}

void BlockHeader::setGenesis() {
  _hash.reset();
  timestamp = alignTimestamp(now_ms_since_1970());
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
void BlockHeaderSigned::setCoinbase(const Pubkey &target, uint64_t amount) {
  coinbase = std::make_shared<Transaction>(scri::Transfer(target, amount));
  coinbase->setNonce();
  coinbase->setExpire(0);
}

void Block::addDataBP(DataBPPtr &data) { datas.push_back(data); }

void Block::addTransaction(TransactionSignedPtr &trx) {
  transactions.push_back(trx);
}

const std::vector<TransactionSignedPtr>& Block::getTransactions() {
  return transactions;
}

void Block::finalize(const Privkey &priv, const crypto::ECCContext &ecc) {
  // 结果数据在执行区块的时候计算出来 并放入 store中
  computeMerkleRoot(std::move(_getHashes()), merkle);
  sign(priv, ecc);
}

bool Block::verifyMerkle() const {
  Hash256 m;
  computeMerkleRoot(std::move(_getHashes()), m);
  return m == merkle;
}

std::vector<Hash256> Block::_getHashes() const {
  uint32_t cnt = 0;
  std::vector<Hash256> results;
  results.push_back(coinbase->getHash());
  *(uint32_t *)&results.back().data()[0] =
      boost::endian::native_to_little(cnt++);
  for (auto trx : transactions) {
    results.push_back(trx->getHash());
    *(uint32_t *)&results.back().data()[0] =
        boost::endian::native_to_little(cnt++);
  }
  for (auto bp : datas) {
    for (auto data : bp->getDatas()) {
      results.push_back(data->getHash(bp->getBP(), height));
      *(uint32_t *)&results.back().data()[0] =
          boost::endian::native_to_little(cnt++);
    }
  }
  return std::move(results);
}

}  // namespace chain
}  // namespace blockmirror
