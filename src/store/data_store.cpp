#include <blockmirror/store/data_store.h>
#include <blockmirror/store/store.h>

namespace blockmirror {
namespace store {

DataStore::DataStore() {
  _path = boost::filesystem::initial_path<boost::filesystem::path>();
}

DataStore::~DataStore() { close(); }

void DataStore::load(const boost::filesystem::path& path) {
  _path = path;
  if (boost::filesystem::exists((_path / "data"))) {
    BinaryReader reader;
    reader.open(_path / "data");
    reader >> _datas;
  }
}

void DataStore::close() {
  BinaryWritter writter;
  writter.open(_path / "data");
  writter << _datas;
}

const store::NewDataPtr DataStore::query(const std::string& name) {
  boost::shared_lock<boost::shared_mutex> lock(_mutex);
  auto it = _datas.find(name);
  if (it == _datas.end()) {
    return nullptr;
  }
  return it->second;
}

bool DataStore::add(const store::NewDataPtr& dataPtr) {
  boost::unique_lock<boost::shared_mutex> lock(_mutex);
  auto p = _datas.insert(std::make_pair(dataPtr->getName(), dataPtr));
  return p.second;
}

}  // namespace store
}  // namespace blockmirror