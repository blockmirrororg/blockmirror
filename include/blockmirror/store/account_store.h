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
  AccountStore();
  ~AccountStore();
  /**
   * @brief 从文件中加载store
   * @param path 路径
   */
  void load(const boost::filesystem::path &path);
  /**
   * @brief 退出程序时需要关闭 主线程 工作线程已关闭
   */
  void close();
  /**
   * @brief 查询账户余额
   */
  uint64_t query(const Pubkey &pubkey);
  /**
   * @brief 转账
   */
  bool transfer(const Pubkey &from, const Pubkey &to, uint64_t amount);
};

}  // namespace store
}  // namespace blockmirror
