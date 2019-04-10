#pragma once

#include <blockmirror/types.h>
#include <blockmirror/chain/data.h>
#include <vector>

namespace blockmirror {
namespace chain {

/**
 * @brief 转账脚本
 * 有且仅有一个签名对 公钥作为源地址
 */
class ScriptTransfer {
 public:
  Pubkey target;
  uint64_t amount;
};

/**
 * @brief 推荐新的出块节点
 * 签名对数量/当前BP数量 必须大于等于 VOTEBP_PERCENT_SIGNER
 */
class ScriptVoteBP {
 public:
  Pubkey bp;
};

/**
 * @brief 删除BP
 * 签名对数量/当前BP数量 必须大于等于 EXITBP_PERCENT_SIGNER
 */
class ScriptExitBP {
 public:
  Pubkey bp;
};

/**
 * @brief 添加新的数据格式
 * 签名对数量/当前BP数量 必须大于等于 VOTENEWDATA_PERCENT_SIGNER
 */
class ScriptNewDataType {
 public:
  std::string name;  // utf-8 32字节 禁止重复
  std::string desc;  // utf-8 256字节
  /**
   * @brief 数据类型
   */
  std::vector<DataType> dataType;
  /**
   * @brief X64 字节码
   * 二进制接口 extern "C" size_t validScript(void *dataArray(rdi), size_t
   * dataCount(rsi), void *dataOut(rdx))
   */
  std::vector<uint8_t> validScript;
  /**
   * 二进制接口 extern "C" bool resultScript(void *dataArray(rdi), size_t
   * dataCount(rsi), void *dataOut(rdx))
   */
  std::vector<uint8_t> resultScript;
};

class ScriptAddData {
 public:
  std::string name;
  std::string type;
};

// class ScriptAddContracts

}  // namespace chain
}  // namespace blockmirror
