#pragma once

/**
 * 关于此目录中的线程安全考虑
 * 1. 已经在跨线程环境中的不能调用非const函数
 * 2. getHash 必须在首次调用后再发送到跨线程环境
 */

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
  template <typename Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(timestamp) & BOOST_SERIALIZATION_NVP(height) &
        BOOST_SERIALIZATION_NVP(previous) & BOOST_SERIALIZATION_NVP(merkle) &
        BOOST_SERIALIZATION_NVP(producer);
  }

 protected:
  mutable Hash256Ptr _hash;

  uint64_t timestamp;
  uint64_t height;
  Hash256 previous;
  Hash256 merkle;
  Pubkey producer;

 public:
  BlockHeader() = default;

  uint64_t getTimestamp() const { return timestamp; }
  uint64_t getHeight() const { return height; }
  const Hash256 &getPrevious() const { return previous; }
  const Hash256 &getMerkle() const { return merkle; }
  const Pubkey &getProducer() const { return producer; }
  const Hash256 &getHash() const;
  void setPrevious(const BlockHeader &parent);
  void setGenesis();
};

class BlockHeaderSigned : public BlockHeader {
 protected:
  friend class blockmirror::serialization::access;
  template <typename Archive>
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
  template <typename Archive>
  void serialize(Archive &ar) {
    BlockHeaderSigned::serialize(ar);
    ar &BOOST_SERIALIZATION_NVP(coinbase) &
        BOOST_SERIALIZATION_NVP(transactions) & BOOST_SERIALIZATION_NVP(datas) &
        BOOST_SERIALIZATION_NVP(result);
  }

 protected:
  TransactionPtr coinbase;  // transfer only
  std::vector<TransactionSignedPtr> transactions;
  std::vector<DataBPPtr> datas;
  std::vector<DataPtr> result;

  // 辅助
  std::unordered_map<Pubkey, DataBPPtr, Hasher> _bpDatas;

 public:
  using BlockHeaderSigned::BlockHeaderSigned;

  /**
   * @brief 增加某BP的一个数据
   * @param bp BP
   * @param data 一项数据
   */
  void addData(const Pubkey &bp, DataSignedPtr &data);
  /**
   * @brief 增加一个BP提交的所有数据
   * @param data BP的数据
   */
  void addDataBP(DataBPPtr &data);
  /**
   * @brief 增加一笔交易
   * @param trx 交易
   */
  void addTransaction(TransactionSignedPtr &trx);
  /**
   * @brief 设置coinbase交易
   * @param target 奖励目标
   * @param amount 奖励金额
   */
  void setCoinbase(const Pubkey &target, uint64_t amount = MINER_AMOUNT);
  /**
   * @brief 完成区块
   * 1. 计算结果数据
   * 2. 计算默克数
   * 3. 签名
   */
  void finalize(const Privkey &priv,
                const crypto::ECCContext &ecc = crypto::ECC);
};

using BlockPtr = std::shared_ptr<Block>;

}  // namespace chain
}  // namespace blockmirror
