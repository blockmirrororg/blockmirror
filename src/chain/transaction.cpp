#include <blockmirror/chain/transaction.h>
#include <blockmirror/serialization/binary_oarchive.h>
#include <blockmirror/serialization/hash_ostream.h>

namespace blockmirror {
namespace chain {

const Hash256 &Transaction::getHash() const {
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

void TransactionSigned::addSign(const Privkey &priv,
                                const crypto::ECCContext &ecc) {
  signatures.push_back(SignaturePair());
  ecc.computePub(priv, signatures.back().signer);
  ecc.sign(priv, getHash(), signatures.back().signature);
}
bool TransactionSigned::verify(const crypto::ECCContext &ecc) {
  auto &hash = getHash();
  for (auto &sigPair : signatures) {
    if (!ecc.verify(sigPair.signer, hash, sigPair.signature)) {
      return false;
    }
  }
  return true;
}

}  // namespace chain
}  // namespace blockmirror