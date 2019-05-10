#pragma once

#include <blockmirror/chain/data.h>
#include <blockmirror/common.h>
#include <blockmirror/serialization/access.h>

namespace blockmirror {
namespace chain {
namespace scri {
// 转账: signer(必须为一个) 给 target 转账 amount
class Transfer {
 protected:
  friend class blockmirror::serialization::access;
  template <typename Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(target) & BOOST_SERIALIZATION_NVP(amount);
  }

 protected:
  Pubkey target;
  uint64_t amount;

 public:
  Transfer() = default;
  Transfer(const Transfer &o) : target(o.target), amount(o.amount) {}
  Transfer(const Pubkey &to, const uint64_t value)
      : target(to), amount(value) {}

  const Pubkey &getTarget() const { return target; }
  uint64_t getAmount() const { return amount; }
};
// 加入BP: signer(>BP_PERCENT_SIGNER) 推荐新的bp
class BPJoin {
 protected:
  friend class blockmirror::serialization::access;
  template <typename Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(bp);
  }

 protected:
  Pubkey bp;

 public:
  BPJoin() = default;
  BPJoin(const BPJoin &b) : bp(b.bp) {}
  BPJoin(const Pubkey &pub) : bp(pub) {}

  const Pubkey &getBP() const { return bp; }
};
// 删除BP: signer(>BP_PERCENT_SIGNER) 踢掉BP
class BPExit {
 protected:
  friend class blockmirror::serialization::access;
  template <typename Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(bp);
  }

 protected:
  Pubkey bp;

 public:
  BPExit() = default;
  BPExit(const BPExit &b) : bp(b.bp) {}
  BPExit(const Pubkey &pub) : bp(pub) {}

  const Pubkey &getBP() const { return bp; }
};
// 新建数据格式: signer(>BP_PERCENT_SIGNER) 新建数据格式
class NewFormat {
 public:
  const static uint8_t TYPE_FLOAT = 1;
  const static uint8_t TYPE_DOUBLE = 2;
  const static uint8_t TYPE_UINT = 3;
  const static uint8_t TYPE_INT = 4;

 protected:
  friend class blockmirror::serialization::access;
  template <typename Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(name) & BOOST_SERIALIZATION_NVP(desc) &
        BOOST_SERIALIZATION_NVP(dataFormat) &
        BOOST_SERIALIZATION_NVP(validScript) &
        BOOST_SERIALIZATION_NVP(resultScript);
  }

 protected:
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

 public:
  NewFormat() = default;
  NewFormat(const NewFormat &b)
      : name(b.name),
        desc(b.desc),
        dataFormat(b.dataFormat),
        validScript(b.validScript),
        resultScript(b.resultScript) {}
  NewFormat(const std::string &n, const std::string &d,
            const std::vector<uint8_t> &format,
            const std::vector<uint8_t> &valid,
            const std::vector<uint8_t> &result)
      : name(n),
        desc(d),
        dataFormat(format),
        validScript(valid),
        resultScript(result) {}

  const std::string &getName() const { return name; }
  const std::string &getDesc() const { return desc; }
  const std::vector<uint8_t> &getDataFormat() const { return dataFormat; }
  const std::vector<uint8_t> &getValidScript() const { return validScript; }
  const std::vector<uint8_t> &getResultScript() const { return resultScript; }
};
// 增加数据: signer(>BP_PERCENT_SIGNER) 增加采集数据
class NewData {
 protected:
  friend class blockmirror::serialization::access;
  template <typename Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(format) & BOOST_SERIALIZATION_NVP(name) &
        BOOST_SERIALIZATION_NVP(desc);
  }

 protected:
  std::string format;
  std::string name;
  std::string desc;

 public:
  NewData() = default;
  NewData(const NewData &o) : format(o.format), name(o.name) {}
  NewData(const std::string &f, const std::string &n, const std::string &d)
      : format(f), name(n), desc(d) {}

  const std::string &getName() const { return name; }
  const std::string &getFormat() const { return format; }
};
using NewDataPtr = std::shared_ptr<NewData>;

}  // namespace scri

using Script = boost::variant<scri::Transfer, scri::BPJoin, scri::BPExit,
                              scri::NewFormat, scri::NewData>;

}  // namespace chain
}  // namespace blockmirror
