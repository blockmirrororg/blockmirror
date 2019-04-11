#pragma once

#include <blockmirror/chain/data.h>
#include <blockmirror/chain/transaction.h>
#include <blockmirror/crypto/ecc.h>
#include <blockmirror/serialization/access.h>
#include <blockmirror/types.h>
#include <boost/serialization/nvp.hpp>

namespace blockmirror {
namespace chain {

class BlockHeader {
 protected:
  friend class blockmirror::serialization::access;
  template <class Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(height) & BOOST_SERIALIZATION_NVP(previous) &
        BOOST_SERIALIZATION_NVP(merkleTransaction) &
        BOOST_SERIALIZATION_NVP(merkleData) & BOOST_SERIALIZATION_NVP(producer);

    SERIALIZE_HASH(ar);
  }

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
 protected:
  friend class blockmirror::serialization::access;
  template <class Archive>
  void serialize(Archive &ar) {
    BlockHeader::serialize(ar);
    ar &BOOST_SERIALIZATION_NVP(signature);
  }

 public:
  Signature signature;

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
  TransactionSigned coinbase;  // transfer only
  std::vector<TransactionSigned> transactions;
  std::vector<DataBP> datas;
  std::vector<DataValue> result;
};

}  // namespace chain
}  // namespace blockmirror