#pragma once

#include <blockmirror/types.h>
#include <blockmirror/chain/transaction.h>

namespace blockmirror {
namespace chain {

class BlockHeader {
 private:
  mutable Hash256Ptr _hash;

 public:
  uint64_t height;
  Hash256 previous;
  Hash256 merkleTransaction;
  Hash256 merkleData;
  Pubkey producer;

  const Hash256 &getHash() const;
};

class BlockHeaderSigned : public BlockHeader {
 public:
  Signature signatures;

  void sign(const Privkey &priv);
  bool verify() const;
};

class Block : public BlockHeaderSigned {
 public:
  std::vector<TransactionSigned> transactions;
};

}  // namespace chain
}  // namespace blockmirror
