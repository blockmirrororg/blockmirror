#include <blockmirror/chain/block.h>
#include <blockmirror/serialization/binary_oarchive.h>
#include <blockmirror/serialization/hash_ostream.h>

namespace blockmirror {
namespace chain {

BlockHeader::BlockHeader() : timestamp(now_ms_since_1970()) {}

const Hash256 &BlockHeader::getHash() const {
  if (_hash) {
    return *_hash;
  }
  serialization::HashOStream oss;
  serialization::BinaryOarchive<decltype(oss)> oa(oss);
  oa << *this;
  _hash.reset(new Hash256());
  oss.getHash(*_hash);
  return *_hash;
}

void BlockHeader::setPrevious(const BlockHeader &parent) {
  _hash.reset();
  previous = parent.getHash();
  height = parent.getHeight() + 1;
  timestamp = now_ms_since_1970();
}

void BlockHeaderSigned::sign(const Privkey &priv,
                             const crypto::ECCContext &ecc) {
  ecc.computePub(priv, producer);
  ecc.sign(priv, getHash(), signature);
}

bool BlockHeaderSigned::verify(const crypto::ECCContext &ecc) const {
  return ecc.verify(producer, getHash(), signature);
}

}  // namespace chain
}  // namespace blockmirror
