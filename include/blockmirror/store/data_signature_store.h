#pragma once

#include <blockmirror/common.h>
#include <blockmirror/chain/data.h>

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
  bool add(const chain::DataSignedPtr& data);
  const chain::DataSignedPtr query(const std::string& name);
  bool remove(const std::string& name);
};

}  // namespace store
}  // namespace blockmirror