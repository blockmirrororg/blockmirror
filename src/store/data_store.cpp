#include <blockmirror/store/data_store.h>
#include <blockmirror/store/store.h>

namespace blockmirror {
namespace store {

DataStore::DataStore() : _loaded(false) {
  _path = boost::filesystem::initial_path<boost::filesystem::path>();
}

DataStore::~DataStore() {
  if (_loaded) close();
}

//void DataStore::load(const boost::filesystem::path& path) {
//  ASSERT(!_loaded);
//  _loaded = true;
//  _path = path;
//  if (boost::filesystem::exists((_path / "data"))) {
//    BinaryReader reader;
//    reader.open(_path / "data");
//    reader >> _datas;
//  }
//}

void DataStore::load(const boost::filesystem::path& path) {
  ASSERT(!_loaded);
  _loaded = true;
  _path = path;
  if (boost::filesystem::exists((_path / "data"))) {
    BinaryReader reader;
    reader.open(_path / "data");
    reader >> *this;
  }
}

//void DataStore::close() {
//  ASSERT(_loaded);
//  _loaded = false;
//  BinaryWritter writter;
//  writter.open(_path / "data");
//  writter << _datas;
//}

void DataStore::close() {
  ASSERT(_loaded);
  _loaded = false;
  BinaryWritter writter;
  writter.open(_path / "data");
  writter << *this;
}

//const store::NewDataPtr DataStore::query(const std::string& name) {
//  boost::shared_lock<boost::shared_mutex> lock(_mutex);
//  auto it = _datas.find(name);
//  if (it == _datas.end()) {
//    return nullptr;
//  }
//  return it->second;
//}

const store::NewDataPtr DataStore::query(const std::string& name) {
  boost::shared_lock<boost::shared_mutex> lock(_mutex);
  auto pos = _container.get<tagName>().find(name);
  if (pos == _container.end()) {
    return nullptr;
  } else {
    return pos->data;
  }
}

//bool DataStore::add(const store::NewDataPtr& dataPtr) {
//  boost::unique_lock<boost::shared_mutex> lock(_mutex);
//  auto p = _datas.insert(std::make_pair(dataPtr->getName(), dataPtr));
//  return p.second;
//}

bool DataStore::add(const store::NewDataPtr& dataPtr) {
  boost::unique_lock<boost::shared_mutex> lock(_mutex);
  auto r = _container.emplace(dataPtr);
  return r.second;
}

//bool DataStore::remove(const std::string& name) {
//  boost::unique_lock<boost::shared_mutex> lock(_mutex);
//  return _datas.erase(name) > 0;
//}

bool DataStore::remove(const std::string& name) {
  boost::unique_lock<boost::shared_mutex> lock(_mutex);
  auto& idx = _container.get<tagName>();
  return idx.erase(name) > 0;
}

//std::vector<store::NewDataPtr>& DataStore::queryEx(std::string format) {
//  std::vector<store::NewDataPtr> v;
//  boost::shared_lock<boost::shared_mutex> lock(_mutex);
//  for (auto pos = _datas.begin(); pos != _datas.end(); ++pos) {
//    if (pos->second->getFormat() == format) {
//      v.emplace_back(pos->second);
//    }
//  }
//  return v;
//}

std::vector<store::NewDataPtr> DataStore::queryEx(std::string format) {
  std::vector<store::NewDataPtr> v;
  boost::shared_lock<boost::shared_mutex> lock(_mutex);
  auto &idx = _container.get<tagFormat>();
  for (auto i = idx.begin(); i != idx.end(); ++i)
  {
    v.emplace_back(i->data);
  }

  return v;
}

}  // namespace store
}  // namespace blockmirror