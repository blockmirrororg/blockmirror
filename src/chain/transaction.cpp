#include <blockmirror/chain/transaction.h>
#include <blockmirror/serialization/binary_oarchive.h>
#include <blockmirror/serialization/hash_ostream.h>
#include <openssl/rand.h>

namespace blockmirror {
namespace chain {

Transaction::Transaction(scri::Transfer &&transfer) : script(transfer) {}
Transaction::Transaction(scri::BPJoin &&join) : script(join) {}
Transaction::Transaction(scri::BPExit &&leave) : script(leave) {}
Transaction::Transaction(scri::NewFormat &&newFormat) : script(newFormat) {}
Transaction::Transaction(scri::NewData &&newData) : script(newData) {}

void Transaction::setNonce() {
  _hash.reset();
  VERIFY(RAND_bytes((unsigned char *)&nonce, sizeof(nonce)));
}
void Transaction::setNonce(uint32_t n) {
  _hash.reset();
  nonce = n;
}
void Transaction::setExpire(uint64_t e) {
  _hash.reset();
  expire = e;
}
const Hash256 &Transaction::getHash() const {
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

void TransactionSigned::addSign(const Privkey &priv,
                                const crypto::ECCContext &ecc) {
  signatures.push_back(SignaturePair());
  ecc.computePub(priv, signatures.back().signer);
  ecc.sign(priv, getHash(), signatures.back().signature);
}
bool TransactionSigned::verify(const crypto::ECCContext &ecc) const {
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