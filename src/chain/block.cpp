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
  serialization::BinaryOarchive<decltype(oss)> oa(oss);
  oa << *this;
  _hash.reset(new Hash256());
  oss.getHash(*_hash);
  return *_hash;
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