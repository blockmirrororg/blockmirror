/**
 * 1. 添加
 * 2. 读取
 * 参考 NewData 交易
 */

#pragma once

#include <blockmirror/chain/script.h>
#include <blockmirror/common.h>

namespace blockmirror {
namespace store {

using NewDataPtr = std::shared_ptr<chain::scri::NewData>;

class DataStore {
 private:
  std::unordered_map<std::string, store::NewDataPtr> _datas;
  // std::unordered_map<std::string, chain::DataPtr> datas_; // add by lvjl

  boost::shared_mutex _mutex;

  boost::filesystem::path _path;

 public:
  DataStore();
  ~DataStore();
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
  const store::NewDataPtr query(const std::string& name);
  /**
   * @brief 添加数据
   */
  bool add(const store::NewDataPtr& dataPtr);
  /**
   * @brief 删除数据
   */
  bool remove(const std::string& name);
  // bool add(const chain::DataPtr& dataPtr); // add by lvjl
};

}  // namespace store
}  // namespace blockmirror