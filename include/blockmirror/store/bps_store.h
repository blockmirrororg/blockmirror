/**
 * 1. 添加
 * 2. 读取
 * 参考 BPJoin 和 BPExit 交易
 */

#pragma once

#include <blockmirror/chain/script.h>
#include <blockmirror/common.h>

namespace blockmirror {
namespace store {

using BPJoinPtr = std::shared_ptr<chain::scri::BPJoin>;
using BPExitPtr = std::shared_ptr<chain::scri::BPExit>;

class BpsStore {
 private:
  std::set<Pubkey, Less> _bps;

  boost::shared_mutex _mutex;

  boost::filesystem::path _path;

 public:
  BpsStore();
  ~BpsStore();
  /**
   * @brief 从文件中加载store
   * @param path 路径
   */
  void load(const boost::filesystem::path& path);
  /**
   * @brief 退出程序时需要关闭 主线程 工作线程已关闭
   */
  void close();
  /**
   * @brief 读取数据
   */
  bool contains(const Pubkey& key);
  /**
   * @brief 添加数据
   */
  bool add(const Pubkey& key);
  /**
   * @brief 减少数据
   */
  bool remove(const Pubkey& key);
  /**
   * @brief 获取BP总数
   */
  uint getBPAmount() { return _bps.size(); }
};

}  // namespace store
}  // namespace blockmirror