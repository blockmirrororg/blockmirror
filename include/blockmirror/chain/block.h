#pragma once

#include <blockmirror/chain/data.h>
#include <blockmirror/chain/transaction.h>
#include <blockmirror/common.h>
#include <blockmirror/crypto/ecc.h>
#include <blockmirror/serialization/access.h>

namespace blockmirror {
namespace chain {

class BlockHeader {
 protected:
  friend class blockmirror::serialization::access;
  template <class Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(timestamp) & BOOST_SERIALIZATION_NVP(height) &
        BOOST_SERIALIZATION_NVP(previous) &
        BOOST_SERIALIZATION_NVP(merkleTransaction) &
        BOOST_SERIALIZATION_NVP(merkleData) & BOOST_SERIALIZATION_NVP(producer);
  }

 protected:
  mutable Hash256Ptr _hash;

  uint64_t timestamp;
  uint64_t height;
  Hash256 previous;
  Hash256 merkleTransaction;
  Hash256 merkleData;
  Pubkey producer;

 public:
  BlockHeader();

  uint64_t getTimestamp() const { return timestamp; }
  uint64_t getHeight() const { return height; }
  const Hash256 &getPrevious() const { return previous; }
  const Hash256 &getMerkleTrx() const { return merkleTransaction; }
  const Hash256 &getMerkleData() const { return merkleData; }
  const Pubkey &getProducer() const { return producer; }
  void setPrevious(const BlockHeader &parent);
  const Hash256 &getHash() const;
};

class BlockHeaderSigned : public BlockHeader {
 protected:
  friend class blockmirror::serialization::access;
  template <class Archive>
  void serialize(Archive &ar) {
    BlockHeader::serialize(ar);
    ar &BOOST_SERIALIZATION_NVP(signature);
  }

 protected:
  Signature signature;

 public:
  using BlockHeader::BlockHeader;

  const Signature &getSignature() const { return signature; }
  void sign(const Privkey &priv, const crypto::ECCContext &ecc = crypto::ECC);
  bool verify(const crypto::ECCContext &ecc = crypto::ECC) const;
};

class Block : public BlockHeaderSigned {
 protected:
  friend class blockmirror::serialization::access;
  template <class Archive>
  void serialize(Archive &ar) {
    BlockHeaderSigned::serialize(ar);
    ar &BOOST_SERIALIZATION_NVP(coinbase) &
        BOOST_SERIALIZATION_NVP(transactions) & BOOST_SERIALIZATION_NVP(datas) &
        BOOST_SERIALIZATION_NVP(result);
  }

 public:
  using BlockHeaderSigned::BlockHeaderSigned;

  TransactionSigned coinbase;  // transfer only
  std::vector<TransactionSigned> transactions;
  std::vector<DataBPPtr> datas;
  std::vector<DataPtr> result;
};

using BlockPtr = std::shared_ptr<Block>;

}  // namespace chain
}  // namespace blockmirror
