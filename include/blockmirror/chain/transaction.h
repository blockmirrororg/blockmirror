#pragma once

#include <blockmirror/types.h>
#include <vector>

namespace blockmirror {
namespace chain {

/**
 * @brief 交易
 * 1. 系统内存会保存 已确认交易 未确认交易 列表 防止重复交易 当交易过期时则从该列表删除
 * 2. 交易的手续费会和过期高度相关 避免占用太多系统内存
 * 3. 交易的手续费会和交易执行脚本相关
 */
class Transaction {
 public:
  uint32_t type;                             // 交易类型
  uint64_t expireHeight;                     // 过期高度 大于这个高度了则丢弃
  uint64_t timestamp;                        // 交易时间戳毫秒
  std::vector<uint8_t> script;               // 交易执行脚本
};

/**
 * @brief 签名后的交易
 * 1. 可以多个签名，再校验有效性的时候需要所有的签名对
 */
class TransactionSigned : public Transaction {
 public:
  std::vector<SignaturePair> signatures;

  void addSign(const Privkey &priv);
  bool verify();
};

}  // namespace chain
}  // namespace blockmirror
