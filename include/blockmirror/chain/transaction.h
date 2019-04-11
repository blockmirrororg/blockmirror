#pragma once

#include <blockmirror/chain/script.h>
#include <blockmirror/crypto/ecc.h>
#include <blockmirror/serialization/access.h>
#include <blockmirror/types.h>
#include <boost/serialization/nvp.hpp>
#include <vector>

namespace blockmirror {
namespace chain {

/**
 * 重放区块(从区块文件重放)
 * 1. 读出一个区块并验证签名
 * 2. 将所有的交易投递到线程开始并发校验签名
 * 3. 将所有的数据投递到线程开始并发校验签名
 * 4. 数据校验完毕后 校验数据计算出的结果是否匹配
 * 5. 交易校验完毕后 顺序执行所有的交易
 * 6. 主线程读出下一个区块并验证签名
 * 7. 等待数据校验和交易执行完进入第2步
 */

/**
 * 出块流程
 * 1.
 */

/**
 * 分叉后切换分叉策略
 * 1. 如果出现某分叉连续落后另外一个<分叉点BP数量>个块则该分叉作废
 * 2. 收到一个块 如果该块落后当前链<分叉点BP数量>个或以上则该块拒绝
 * 3. 收到一个非拒绝的块 如果是当前链则应用该块
 * 4. 如果是非当前链则先放入孤块池，如果块高度大于当前链则尝试切换
 */

/**
 * @brief 交易
 * 1. 系统内存会保存 已确认交易 未确认交易 列表 防止重复交易
 * 当交易过期时则从该列表删除
 * 2. 交易的手续费会和过期高度相关 避免占用太多系统内存
 * 3. 交易的手续费会和交易执行脚本相关
 */
class Transaction {
 protected:
  friend class blockmirror::serialization::access;
  template <class Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(expire) & BOOST_SERIALIZATION_NVP(nonce) &
        BOOST_SERIALIZATION_NVP(script);

    SERIALIZE_HASH(ar);
  }

 private:
  mutable Hash256Ptr _hash;

 public:
  uint64_t expire;  // 过期高度 大于这个高度了则丢弃
  uint32_t nonce;   // 交易随机数
  Script script;    // 交易执行脚本

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

 public:
  std::vector<SignaturePair> signatures;

  void addSign(const Privkey &priv,
               const crypto::ECCContext &ecc = crypto::ECC);
  bool verify(const crypto::ECCContext &ecc = crypto::ECC);
};

}  // namespace chain
}  // namespace blockmirror
