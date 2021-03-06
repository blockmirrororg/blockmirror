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

  uint64_t timestamp;  // 区块产生的时间戳
  uint64_t height;     // 区块的当前高度
  Hash256 previous;    // 上一个区块的HASH值
  Hash256 merkle;      // 区块所有交易和所有数据的默克尔根
  Pubkey producer;     // 区块生产者的公钥

 public:
  BlockHeader() = default;

  uint64_t getTimestamp() const { return timestamp; }
  void setTimestamp(uint64_t t) { timestamp = t; }
  uint64_t getHeight() const { return height; }
  const Hash256 &getPrevious() const { return previous; }
  const Hash256 &getMerkle() const { return merkle; }
  const Pubkey &getProducer() const { return producer; }
  const Hash256 &getHash() const;
  const Hash256Ptr &getHashPtr() const {
    if (!_hash) getHash();
    return _hash;
  }
  void setPrevious(const BlockHeader &parent);
  void setGenesis();
};

class BlockHeaderSigned : public BlockHeader {
 protected:
  friend class blockmirror::serialization::access;
  template <typename Archive>
  void serialize(Archive &ar) {
    BlockHeader::serialize(ar);
    ar &BOOST_SERIALIZATION_NVP(coinbase) & BOOST_SERIALIZATION_NVP(signature);
  }

 protected:
  Signature signature;      // 区块生产者对这个区块的签名
  TransactionPtr coinbase;  // 这个区块的奖励交易

 public:
  using BlockHeader::BlockHeader;

  const Signature &getSignature() const { return signature; }
  void sign(const Privkey &priv, const crypto::ECCContext &ecc = crypto::ECC);
  bool verify(const crypto::ECCContext &ecc = crypto::ECC) const;

  /**
   * @brief 设置coinbase交易
   * @param target 奖励目标
   * @param amount 奖励金额
   */
  void setCoinbase(const Pubkey &target, uint64_t amount = MINER_AMOUNT);
  const TransactionPtr &getCoinbase() const { return coinbase; }
  TransactionPtr &getCoinbase() { return coinbase; }
};
using BlockHeaderSignedPtr = std::shared_ptr<BlockHeaderSigned>;

class Block : public BlockHeaderSigned {
 protected:
  friend class blockmirror::serialization::access;
  template <typename Archive>
  void serialize(Archive &ar) {
    BlockHeaderSigned::serialize(ar);
    ar &BOOST_SERIALIZATION_NVP(transactions) & BOOST_SERIALIZATION_NVP(datas);
  }

 protected:
  std::vector<TransactionSignedPtr> transactions;  // 区块中所有的交易
  std::vector<DataBPPtr> datas;                    // 区块中所有的数据
  // std::vector<DataPtr> result; // FIXME: 结果数据可能不需要放在区块中
  // 应该直接更新到store

  std::vector<Hash256> _getHashes() const;

 public:
  using BlockHeaderSigned::BlockHeaderSigned;

  /**
   * @brief 检测默克ROOT是否正确
   * @return true
   * @return false
   */
  bool verifyMerkle() const;
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
   * @brief 获取区块的交易
   */
  const std::vector<TransactionSignedPtr> &getTransactions();
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

struct BlockLess {
  bool operator()(const Block &x, const Block &y) const {
    if (x.getHeight() == y.getHeight()) {
      return x.getHash() < y.getHash();
    }
    return x.getHeight() < y.getHeight();
  }
  bool operator()(const BlockPtr &x, const BlockPtr &y) const {
    if (!x) return true;
    if (!y) return false;
    return operator()(*x, *y);
  }
};

}  // namespace chain
}  // namespace blockmirror
