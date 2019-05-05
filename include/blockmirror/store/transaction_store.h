/**
 * 交易池 保存未过期的交易 防止交易重复
 * 1. 产生区块时：将所有未打包未确认的交易打包进去
 * 2. 执行新区块时：将区块所有交易标记为已打包
 * 过期高度小于等于区块高度的交易从交易池删除
 * 3. 当回滚区块时：将区块所有交易标记为未打包 如果在交易池中不存在重新添加回去
 * 4. 网络接口收到交易时：检测交易合法且可执行则加入交易池
 * 5. 初始化和关闭时：从文件加载或者写入文件
 * 需要实现接口:
 * 1. 根据HASH256判断交易是否存在<读>
 * 2. 添加交易并设置标记是否打包过<写>
 * 3. 修改交易标记<写>
 * 4. 根据高度移除交易<写>
 * 5. 拷贝全部未打包交易
 */
#pragma once

#include <blockmirror/chain/transaction.h>
#include <blockmirror/common.h>
#include <blockmirror/serialization/access.h>

namespace blockmirror {
namespace store {

class TransactionMarked {
 protected:
  friend class blockmirror::serialization::access;
  template <typename Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(_transPtr) & BOOST_SERIALIZATION_NVP(_mark);
  }

 protected:
  chain::TransactionPtr _transPtr;
  uint8_t _mark;

 public:
  TransactionMarked() {
    _transPtr = nullptr;
    _mark = 0;
  }
  TransactionMarked(chain::TransactionPtr t, uint8_t m)
      : _transPtr(t), _mark(m) {}
  void setMark(uint8_t m) { _mark = m; };
  const chain::TransactionPtr getTransactionPtr() const { return _transPtr; }
  const uint8_t getMark() const { return _mark; }
};

using TransactionMarkedPtr = std::shared_ptr<TransactionMarked>;

class TransactionStore {
 private:
  std::unordered_map<Hash256, TransactionMarkedPtr, blockmirror::Hasher,
                     blockmirror::EqualTo>
      _trs;
  boost::shared_mutex _mutex;
  boost::filesystem::path _path;

 public:
  const static uint8_t UNPACK_STATUS = 0;
  const static uint8_t PACK_STATUS = 1;

 public:
  TransactionStore();
  ~TransactionStore();
  /**
   * @brief 从文件中加载store
   */
  void load(const boost::filesystem::path &path);
  /**
   * @brief 退出程序时需要关闭 主线程 工作线程已关闭
   */
  void close();
  /**
   * @brief 判断交易是否存在
   */
  bool exist(const Hash256 &h);
  /**
   * @brief 添加交易并设置标记是否打包过
   */
  bool add(const chain::TransactionPtr &tPtr, uint8_t m = UNPACK_STATUS);
  /**
   * @brief 修改交易标记
   */
  bool modify(const Hash256 &h, uint8_t m);
  /**
   * @brief 根据高度移除交易
   */
  size_t remove(const uint64_t &h);
  /**
   * @brief 拷贝全部未打包交易
   */
  std::vector<chain::TransactionPtr> copy();
};

}  // namespace store
}  // namespace blockmirror
