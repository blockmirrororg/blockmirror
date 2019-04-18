#pragma once

#include <blockmirror/chain/block.h>

namespace blockmirror {
namespace store {

class BlockStore {
 private:
  std::list<chain::BlockPtr> _lru;
  std::unordered_map<Hash256Ptr, chain::BlockPtr, Hasher> _cached;
  std::unordered_map<Hash256Ptr, uint64_t, Hasher> _index;

 public:
  BlockStore();
  ~BlockStore();
  /**
   * @brief 加载STORE
   * @param path 路径
   */
  void load(const boost::filesystem::path &path);
  /**
   * @brief 退出程序时需要关闭
   */
  void closeSave();
  /**
   * @brief 判断区块是否存在
   * 
   * @param hash 区块哈希
   * @return true 存在
   * @return false 不存在
   */
  bool hasBlock(const Hash256Ptr &hash);
  /**
   * @brief 查找区块
   * @param hash 区块哈希
   * @return const BlockPtr
   */
  chain::BlockPtr findBlock(const Hash256Ptr &hash);
  /**
   * @brief 添加区块
   * @param block 区块
   * @return bool 如果存在(HASH重复)则无法增加
   */
  bool addBlock(chain::BlockPtr &block);
  /**
   * @brief 是否需要切换分支 线程安全
   * @param head 当前区块
   * @param fork 分叉区块
   * @param back 需要回退的区块列表 包含当前区块
   * @param forward 需要应用的区块列表 包含分叉区块
   */
  void shouldSwitch(const chain::BlockPtr &head, const chain::BlockPtr &fork,
                  std::vector<chain::BlockPtr> &back,
                  std::vector<chain::BlockPtr> &forward);
};

}  // namespace store
}  // namespace blockmirror
