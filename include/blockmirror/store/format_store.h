/**
 * 1. 添加格式<写>
 * 2. 读取格式<读>
 * 参考 NewFormat 交易
 */
#pragma once

#include <blockmirror/chain/script.h>
#include <blockmirror/common.h>

namespace blockmirror {
namespace store {

using NewFormatPtr = std::shared_ptr<chain::scri::NewFormat>;

class FormatStore {
 private:
  std::unordered_map<std::string, store::NewFormatPtr> _formats;

  boost::shared_mutex _mutex;

  boost::filesystem::path _path;

 public:
  FormatStore();
  ~FormatStore();
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
   * @brief 读取格式
   */
  const store::NewFormatPtr query(const std::string& name);
  /**
   * @brief 添加格式
   */
  bool add(const store::NewFormatPtr& formatPtr);
};

}  // namespace store
}  // namespace blockmirror