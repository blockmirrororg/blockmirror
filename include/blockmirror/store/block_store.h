/**
 * 区块池
 */
#pragma once

#include <blockmirror/chain/block.h>

namespace blockmirror {
namespace store {

class BlockStore {
  static inline uint32_t getOffset(const uint64_t &index) {
    return (uint32_t)index;
  }
  static inline uint32_t getFile(const uint64_t &index) {
    return (uint32_t)(index >> 32);
  }
  static inline uint64_t makeIndex(const uint32_t &offset,
                                   const uint32_t file) {
    return (uint64_t)offset | ((uint64_t)file << 32);
  }

 private:
  // 写入文件的块
  std::unordered_map<Hash256Ptr, uint64_t, Hasher, EqualTo> _index;

  // 缓存在内存的块
  std::set<chain::BlockPtr, chain::BlockLess> _ordered;
  std::unordered_map<Hash256Ptr, chain::BlockPtr, Hasher, EqualTo> _cached;

  boost::shared_mutex _mutex;

  uint32_t _currentFileIndex;
  boost::mutex _fileMutex;

  boost::filesystem::path _path;
  bool _loaded;

  chain::BlockPtr _loadBlock(uint64_t index);
  uint64_t _saveBlock(chain::BlockPtr block);
 public:
  BlockStore();
  ~BlockStore();
  /**
   * @brief 加载STORE 主线程 工作线程未启动
   * @param path 路径
   */
  void load(const boost::filesystem::path &path);
  /**
   * @brief 退出程序时需要关闭 主线程 工作线程已关闭
   */
  void close();
  /**
   * @brief 判断区块是否存在 线程安全
   *
   * @param hash 区块哈希
   * @return true 存在
   * @return false 不存在
   */
  bool contains(const Hash256Ptr &hash);
  bool contains(const Hash256 &hash);
  /**
   * @brief 查找区块 线程安全(不得调用block的非const函数)
   * @param hash 区块哈希
   * @return const BlockPtr
   */
  chain::BlockPtr getBlock(const Hash256Ptr &hash);
  chain::BlockPtr getBlock(const Hash256 &hash);
  /**
   * @brief 添加区块 线程安全
   * @param block 区块
   * @return bool 如果存在(HASH重复)则无法增加
   */
  bool addBlock(chain::BlockPtr &block);
  /**
   * @brief 将过于老旧的区块从缓存中清除写入文件中 线程安全
   */
  void flushBlock(size_t storeLimit = BLOCKSTORE_LIMIT);
  /**
   * @brief 是否需要切换分支 线程安全
   * @param head 当前区块
   * @param fork 分叉区块
   * @param back 需要回退的区块列表 包含当前区块
   * @param forward 需要应用的区块列表 包含分叉区块
   * @return true 需要切换
   * @return false 不需要
   */
  bool shouldSwitch(const chain::BlockPtr &head, const chain::BlockPtr &fork,
                    std::vector<chain::BlockPtr> &back,
                    std::vector<chain::BlockPtr> &forward);

  void saveToMongo(chain::BlockPtr& block);
};

}  // namespace store
}  // namespace blockmirror
