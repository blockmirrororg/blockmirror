#pragma once

#include <blockmirror/chain/script.h>
#include <blockmirror/common.h>
#include <blockmirror/crypto/ecc.h>
#include <blockmirror/serialization/access.h>

namespace blockmirror {
namespace chain {

class Transaction {
 protected:
  friend class blockmirror::serialization::access;
  template <class Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(expire) & BOOST_SERIALIZATION_NVP(nonce) &
        BOOST_SERIALIZATION_NVP(script);
  }

 protected:
  mutable Hash256Ptr _hash;

  uint64_t expire;  // 过期高度 大于这个高度了则丢弃
  uint32_t nonce;   // 交易随机数
  Script script;    // 交易执行脚本

 public:
  Transaction(script::Transfer &&transfer);
  Transaction(script::BPJoin &&join);
  Transaction(script::BPExit &&leave);
  Transaction(script::NewFormat &&newFormat);
  Transaction(script::NewData &&newData);
  Transaction() = default;

  void setNonce(uint32_t n);
  void setExpire(uint64_t e);
  void setNonce();

  uint64_t getExpire() const { return expire; }
  uint32_t getNonce() const { return nonce; }
  const Script &getScript() const { return script; }

  const Hash256 &getHash() const;
};

struct SignaturePair {
  template <class Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(signer) & BOOST_SERIALIZATION_NVP(signature);
  }

  Pubkey signer;
  Signature signature;
};

/**
 * @brief 签名后的交易
 * 1. 可以多个签名，再校验有效性的时候需要所有的签名对
 */
class TransactionSigned : public Transaction {
 protected:
  friend class blockmirror::serialization::access;
  template <class Archive>
  void serialize(Archive &ar) {
    Transaction::serialize(ar);
    ar &BOOST_SERIALIZATION_NVP(signatures);
  }

 protected:
  std::vector<SignaturePair> signatures;

 public:
  using Transaction::Transaction;

  const std::vector<SignaturePair> &getSignatures() const { return signatures; }

  void addSign(const Privkey &priv,
               const crypto::ECCContext &ecc = crypto::ECC);
  bool verify(const crypto::ECCContext &ecc = crypto::ECC);
};
using TransactionSignedPtr = std::shared_ptr<Transaction>;

}  // namespace chain
}  // namespace blockmirror
