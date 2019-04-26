/**
 * 区块池
 */
#pragma once

#include <blockmirror/common.h>

namespace blockmirror {
namespace store {

class AccountStore {
 private:
  // 账户和账户余额
  std::unordered_map<Pubkey, uint64_t> _accounts;

  boost::shared_mutex _mutex;

  boost::filesystem::path _path;

 public:
  /**
   * @brief 从文件中加载store
   * @param path 路径
   */
  void load(const boost::filesystem::path &path) {
    // 从文件中加载 _accounts
  }
  /**
   * @brief 退出程序时需要关闭 主线程 工作线程已关闭
   */
  void close() {
    // 保存到文件 _accounts
  }
  uint64_t query(const Pubkey &pubkey) {
    boost::shared_lock<boost::shared_mutex> lock(_mutex);
    auto i = _accounts.find(pubkey);
    if (i == _accounts.end()) return 0;
    return i->second;
  }
  bool transfer(const Pubkey &from, const Pubkey &to, uint64_t amount) {
    boost::upgrade_lock<boost::shared_mutex> lock(_mutex);
    auto fromIter = _accounts.find(from);
    auto toIter = _accounts.find(to);
    if (fromIter == _accounts.end() || fromIter->second < amount) {
      return false;
    }
    boost::upgrade_to_unique_lock<boost::shared_mutex> upgrade(lock);
    if (toIter == _accounts.end()) {
      auto insert = _accounts.insert(std::make_pair(to, amount));
      if (!insert.second) return false;
    } else {
      toIter->second += amount;
    }
    fromIter->second -= amount;
    return true;
  }
};

}  // namespace store
}  // namespace blockmirror
