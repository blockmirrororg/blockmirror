/**
 * 区块池
 * 
 * 出块流程:
 * 1. 主线程出块完 addBlock
 * 2. 向所有连接广播该块的HASH
 * 3. 连接收到该HASH后首先查看本机是否存在 不存在则加入pending中向对方发送 GET_BLOCK 消息
 * 4. 工作线程的定时器将缓存中的块按高度写入到文件中
 * 5. 工作线程的定时器会清除比较老的块
 * 
 * 读取区块流程:
 * 1. 如果缓存中存在直接从缓存中读取
 * 2. 否则从文件中加载
 * 
 * 网路区块流程:
 * 1. 工作线程收到广播块的消息 确认本机
 * 
 * 当发生分叉需要回滚时 能快速查找到区块
 * 1. 添加区块时区块立即永久写入文件：前缀可以添加区块大小和校验码
 * 投递到工作线程写入
 * 2. 区块池中保存允许最多回滚高度的所有区块
 * 3. 可以根据两个区块查找出分叉
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
  std::unordered_map<Hash256Ptr, uint64_t> _index;

  // 缓存在内存的块
  std::set<chain::BlockPtr, chain::BlockLess> _ordered;
  std::unordered_map<Hash256Ptr, chain::BlockPtr, Hasher, EqualTo> _cached;

  boost::shared_mutex _mutex;

  uint32_t _currentFileIndex;
  std::ofstream _currentFile;
  boost::mutex _fileMutex;

  boost::filesystem::path _path;

  chain::BlockPtr _loadBlock(uint64_t index);
  uint64_t _saveBlock(chain::BlockPtr block);
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
  void close();
  /**
   * @brief 判断区块是否存在
   *
   * @param hash 区块哈希
   * @return true 存在
   * @return false 不存在
   */
  bool contains(const Hash256Ptr &hash);
  bool contains(const Hash256 &hash);
  /**
   * @brief 查找区块
   * @param hash 区块哈希
   * @return const BlockPtr
   */
  chain::BlockPtr getBlock(const Hash256Ptr &hash);
  chain::BlockPtr getBlock(const Hash256 &hash);
  /**
   * @brief 添加区块
   * @param block 区块
   * @return bool 如果存在(HASH重复)则无法增加
   */
  bool addBlock(chain::BlockPtr &block);
  /**
   * @brief 将过于老旧的区块从缓存中清除写入文件中
   */
  void flushBlock(size_t storeLimit = BLOCKSTORE_LIMIT);
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
