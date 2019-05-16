/**
 * 管理BP的状态 <只能在主线程中使用>
 * 1. 负责管理当前BP的集合
 * 2. 根据公钥判断是否在BP集合中
 * 3. 根据公钥查找在BP集合中的位置
 * 4. 添加和删除BP
 * 5. 压入和弹出BP变更记录
 * 6. 计算某个BP出块的时间偏移

出块机制
  1) BP列表中的BP循环出块 例如4个BP出块序号为(0, 1, 2, 3, 0, 1, 2, 3)
  2) 出块间隔为 BLOCK_PER_MS 块的时间戳必须和出块间隔对齐
  3) 出块时间 第一个块则立即出 并变更记录顶部设置为 (出块序号(0),
块时间戳(出块间隔对齐))
  4) 否则根据公式
    当前出块序号 = ((当前时间戳 - 顶部记录时间戳) / 出块间隔 + 顶部出块序号) % s
    某BP的出块延迟 = (某BP出块序号 - 当前出块序号) * 出块间隔 - 当前时间对齐尾数

 */

#pragma once

#include <blockmirror/chain/script.h>
#include <blockmirror/common.h>

namespace blockmirror {
namespace store {

class BpsStore {
 private:
  // BP变更记录
  struct ChangeRecords {
    template <typename Archive>
    void serialize(Archive& ar) {
      ar& timestamp& index;
    }
    ChangeRecords(){};
    ChangeRecords(uint64_t idx, uint64_t t) : index(idx), timestamp(t) {}
    uint64_t timestamp;  // 变更时间戳 可对应到具体区块
    uint64_t index;      // 变更时该区块的BP位置
  };
  std::vector<ChangeRecords> _changes;
  // BP列表
  std::vector<Pubkey> _bps;

  boost::filesystem::path _path;
  bool _loaded;

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
  int find(const Pubkey& key);
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
  uint32_t getBPAmount() { return (uint32_t)_bps.size(); }
  /**
   * @brief 计算某个BP出块的时间偏移
   *
   * @param key 出块公钥
   * @param now 当前时间戳
   * @return int 小于0不存在 大于0需要的偏移
   */
  int getBPDelay(const Pubkey& key, uint64_t now = now_ms_since_1970());

  // 根据时间计算该时间所对应的出块者序号
  inline uint64_t getSlotByTime(uint64_t now) {
    ASSERT(!_changes.empty());
    ASSERT(!_bps.empty());
    auto& top = _changes.back();
    ASSERT(isAlignedTime(top.timestamp));
    return (((now - top.timestamp) / BLOCK_PER_MS) + top.index) % _bps.size();
  }
  /**
   * @brief BP变动时应该将改变压入链中
   *
   * @param idx 造成改变所在的位置
   * @param timestamp 时间戳
   */
  void pushBpChange(uint64_t idx, uint64_t timestamp);
  void pushBpChange(const Pubkey& producer, uint64_t timestamp);
  void popBpChange();
};

}  // namespace store
}  // namespace blockmirror