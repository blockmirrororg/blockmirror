#pragma once

#include <blockmirror/chain/script.h>
#include <blockmirror/common.h>

namespace blockmirror {
namespace store {

using NewDataPtr = std::shared_ptr<chain::scri::NewData>;

class DataSignatureStore {
public:
  bool add(const NewDataPtr& data);
  const store::NewDataPtr query(const std::string& name);
  bool remove(const std::string& name);

 private:
  Signature signature;
  std::map<std::string, store::NewDataPtr> _datas;
  boost::shared_mutex _mutex;
  boost::filesystem::path _path;
 };

}  // namespace store
}  // namespace blockmirror