#pragma once

#include <blockmirror/chain/data.h>
#include <blockmirror/chain/transaction.h>
#include <blockmirror/serialization/access.h>
#include <blockmirror/types.h>

#include <boost/serialization/nvp.hpp>

namespace blockmirror {
namespace chain {

class BlockHeader {
  friend class blockmirror::serialization::access;
  template <class Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(height) & BOOST_SERIALIZATION_NVP(previous) &
        BOOST_SERIALIZATION_NVP(merkleTransaction) &
        BOOST_SERIALIZATION_NVP(merkleData) & BOOST_SERIALIZATION_NVP(producer);
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
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive &ar, const unsigned int /* file_version */) {
    boost::serialization::base_object<BlockHeader>(*this);
    ar &BOOST_SERIALIZATION_NVP(signature);
  }

 private:
  mutable bool _verified;

 public:
  Signature signature;

  void sign(const Privkey &priv);
  bool verify() const;
};

class Block : public BlockHeaderSigned {
 public:
  TransactionSigned coinbase;  // transfer only
  std::vector<TransactionSigned> transactions;
  DataStore datas;
  DataResult result;
};

}  // namespace chain
}  // namespace blockmirror
