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
 * 2. 添加交易<写>
 * 3. 修改交易打包高度<写>
 * 4. 根据过期高度移除交易<写>
 * 5. 拷贝并移除全部打包高度为0的交易<读>
 * 6.
 */
#pragma once

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

#include <blockmirror/chain/transaction.h>
#include <blockmirror/common.h>
#include <blockmirror/serialization/access.h>

namespace blockmirror {
namespace store {

class TransactionStore {
 protected:
  friend class blockmirror::serialization::access;

  template <typename Archive,
            typename std::enable_if<Archive::IsSaving::value, int>::type = 0>
  void serialize(Archive &ar) {
    ar << (uint32_t)_container.size();
    for (auto i = _container.begin(); i != _container.end(); ++i) {
      ar << *i;
    }
  }

  template <typename Archive,
            typename std::enable_if<!Archive::IsSaving::value, int>::type = 0>
  void serialize(Archive &ar) {
    uint32_t size;
    ar >> size;
    if (size > SERIALIZER_MAX_SIZE_T) {
      throw std::runtime_error("TransactionStore::serialize bad size");
    }
    for (uint32_t i = 0; i < size; i++) {
      TransactionItem item(nullptr, 0);
      ar >> item;
      _container.insert(item);
    }
  }

 private:
  struct TransactionItem {
    friend class blockmirror::serialization::access;
    template <typename Archive>
    void serialize(Archive &ar) {
      ar &transaction &height;
    }
    chain::TransactionSignedPtr transaction;
    uint64_t height;

    TransactionItem(const chain::TransactionSignedPtr &trx, uint64_t h)
        : transaction(trx), height(h) {}

    const Hash256Ptr hash() const { return transaction->getHashPtr(); }

    uint64_t expire() const { return transaction->getExpire(); }
  };

  struct tagHash {};
  struct tagHeight {};
  struct tagExpire {};

  typedef boost::multi_index::multi_index_container<
      TransactionItem,
      boost::multi_index::indexed_by<
          boost::multi_index::hashed_unique<boost::multi_index::tag<tagHash>,
                                            BOOST_MULTI_INDEX_CONST_MEM_FUN(
                                                TransactionItem,
                                                const Hash256Ptr, hash),
                                            Hasher, EqualTo>,
          boost::multi_index::ordered_non_unique<
              boost::multi_index::tag<tagHeight>,
              BOOST_MULTI_INDEX_MEMBER(TransactionItem, uint64_t, height)>,
          boost::multi_index::ordered_non_unique<
              boost::multi_index::tag<tagExpire>,
              BOOST_MULTI_INDEX_CONST_MEM_FUN(TransactionItem, uint64_t,
                                              expire)> > >
      TransactionContainer;

  TransactionContainer _container;
  boost::shared_mutex _mutex;
  boost::filesystem::path _path;
  bool _loaded;

 public:
  TransactionStore() : _loaded(false) {}
  ~TransactionStore() { if (_loaded) close(); }
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
  bool contains(const Hash256Ptr &h);
  /**
   * @brief 添加或者修改交易
   *
   * @param trx 交易
   * @param height 打包区块 0表示未打包
   * @return true 添加成功
   * @return false 添加失败修改打包高度
   */
  bool add(const chain::TransactionSignedPtr &trx, uint64_t height = 0);
  /**
   * @brief 弹出所有未打包的交易
   *
   * @return std::vector<chain::TransactionSignedPtr>
   */
  std::vector<chain::TransactionSignedPtr> popUnpacked();
  /**
   * @brief 删除已经超时的交易
   *
   * @param height 当前高度
   */
  void removeExpired(uint64_t height);
  /**
   * @brief 删除交易
   *
   * @param trx 交易
   */
  bool remove(const chain::TransactionSignedPtr &trx);
};

}  // namespace store
}  // namespace blockmirror
