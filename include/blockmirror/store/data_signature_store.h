#pragma once

#include <blockmirror/common.h>
#include <blockmirror/chain/data.h>

namespace blockmirror {
namespace store {

class DataSignatureStore {
public:
  bool add(const chain::DataSignedPtr& data);
  const chain::DataSignedPtr& query(std::string& name);
  bool remove(std::string& name);

 private:
  std::map<std::string, chain::DataSignedPtr> _datas;
  boost::shared_mutex _mutex;
  boost::filesystem::path _path;
 };

}  // namespace store
}  // namespace blockmirror