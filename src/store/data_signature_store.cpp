
#include <blockmirror/store/data_signature_store.h>

namespace blockmirror {
namespace store {

bool DataSignatureStore::add(const NewDataPtr& data) {
  boost::unique_lock<boost::shared_mutex> lock(_mutex);
  auto ret = _datas.insert(std::make_pair(data->getName(), data));
  if (!ret.second) {
    // 插入失败则修改
    ret.first->second = data;
  }
  return ret.second;
}

const store::NewDataPtr DataSignatureStore::query(const std::string& name) {
  boost::unique_lock<boost::shared_mutex> lock(_mutex);
  auto it = _datas.find(name);
  if (it == _datas.end()) {
    return nullptr;
  }
  return it->second;
}

bool DataSignatureStore::remove(const std::string& name)
{
  boost::unique_lock<boost::shared_mutex> lock(_mutex);
  return _datas.erase(name) > 0;
}

}  // namespace store
}  // namespace blockmirro