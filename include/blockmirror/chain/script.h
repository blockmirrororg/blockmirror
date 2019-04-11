#pragma once

#include <blockmirror/chain/data.h>
#include <blockmirror/serialization/access.h>
#include <blockmirror/types.h>
#include <boost/serialization/nvp.hpp>
#include <boost/variant.hpp>
#include <vector>

namespace blockmirror {
namespace chain {
namespace script {
// 转账: signer(必须为一个) 给 target 转账 amount
class Transfer {
 protected:
  friend class blockmirror::serialization::access;
  template <class Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(target) & BOOST_SERIALIZATION_NVP(amount);
  }

 public:
  Pubkey target;
  uint64_t amount;
};
// 加入BP: signer(>BP_PERCENT_SIGNER) 推荐新的bp
class BPJoin {
 protected:
  friend class blockmirror::serialization::access;
  template <class Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(bp);
  }

 public:
  Pubkey bp;
};
// 删除BP: signer(>BP_PERCENT_SIGNER) 踢掉BP
class BPExit {
 protected:
  friend class blockmirror::serialization::access;
  template <class Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(bp);
  }

 public:
  Pubkey bp;
};
// 新建数据格式: signer(>BP_PERCENT_SIGNER) 新建数据格式
class NewFormat {
 protected:
  friend class blockmirror::serialization::access;
  template <class Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(name) & BOOST_SERIALIZATION_NVP(desc) &
        BOOST_SERIALIZATION_NVP(dataFormat) &
        BOOST_SERIALIZATION_NVP(validScript) &
        BOOST_SERIALIZATION_NVP(resultScript);
  }

 public:
  std::string name;  // utf-8 32字节 禁止重复
  std::string desc;  // utf-8 256字节
  /**
   * @brief 数据类型
   */
  std::vector<uint8_t> dataFormat;
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
// 增加数据: signer(>BP_PERCENT_SIGNER) 增加采集数据
class NewData {
 protected:
  friend class blockmirror::serialization::access;
  template <class Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(format) & BOOST_SERIALIZATION_NVP(name);
  }

 public:
  uint64_t format;
  std::string name;
};

}  // namespace script

using Script = boost::variant<script::Transfer, script::BPJoin, script::BPExit,
                              script::NewFormat, script::NewData>;

}  // namespace chain
}  // namespace blockmirror
