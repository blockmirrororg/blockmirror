#include <blockmirror/store/data_signature_store.h>
#include <blockmirror/store/store.h>

namespace blockmirror {
namespace store {

DataSignatureStore::DataSignatureStore() : _loaded(false) {
  _path = boost::filesystem::initial_path<boost::filesystem::path>();
}
DataSignatureStore::~DataSignatureStore() {
  if (_loaded) close();
}

void DataSignatureStore::load(const boost::filesystem::path& path) {
  ASSERT(!_loaded);
  _loaded = true;
  _path = path;
  if (boost::filesystem::exists(_path / "datasigned")) {
    BinaryReader reader;
    reader.open(_path / "datasigned");
    reader >> _datas;
  }
}

void DataSignatureStore::close() {
  ASSERT(_loaded);
  _loaded = false;
  BinaryWritter writter;
  writter.open(_path / "datasigned");
  writter << _datas;
}

bool DataSignatureStore::add(const chain::DataSignedPtr& data) {
  boost::unique_lock<boost::shared_mutex> lock(_mutex);
  auto ret = _datas.insert(std::make_pair(data->getName(), data));
  if (!ret.second) {
    // 插入失败则修改
    ret.first->second = data;
  }
  return ret.second;
}

const chain::DataSignedPtr DataSignatureStore::query(const std::string& name) {
  boost::unique_lock<boost::shared_mutex> lock(_mutex);
  auto it = _datas.find(name);
  if (it == _datas.end()) {
    return nullptr;
  }
  return it->second;
}

bool DataSignatureStore::remove(const std::string& name) {
  boost::unique_lock<boost::shared_mutex> lock(_mutex);
  return _datas.erase(name) > 0;
}

std::vector<chain::DataSignedPtr> DataSignatureStore::pop() {
  boost::unique_lock<boost::shared_mutex> lock(_mutex);
  std::vector<chain::DataSignedPtr> v;
  for (auto i : _datas) {
    v.push_back(i.second);
  }
  _datas.clear();
  return std::move(v);
}

}  // namespace store
}  // namespace blockmirror