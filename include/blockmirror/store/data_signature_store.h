#pragma once

#include <blockmirror/chain/data.h>
#include <blockmirror/common.h>

namespace blockmirror {
namespace store {

class DataSignatureStore {
 private:
  std::unordered_map<std::string, chain::DataSignedPtr> _datas;
  boost::shared_mutex _mutex;
  boost::filesystem::path _path;
  bool _loaded;

 public:
  DataSignatureStore();
  ~DataSignatureStore();
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
   * @brief 添加数据
   */
  bool add(const chain::DataSignedPtr& data);
  /**
   * @brief 查询数据
   */
  const chain::DataSignedPtr query(const std::string& name);
  /**
   * @brief 删除数据
   */
  bool remove(const std::string& name);
  /**
   * @brief 弹出所有数据
   *
   * @return std::vector<chain::DataSignedPtr>
   */
  std::vector<chain::DataSignedPtr> pop();
};

}  // namespace store
}  // namespace blockmirror