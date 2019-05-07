#include <blockmirror/serialization/binary_iarchive.h>
#include <blockmirror/serialization/binary_oarchive.h>
#include <blockmirror/store/data_store.h>

namespace blockmirror {
namespace store {

DataStore::DataStore() {
  _path = boost::filesystem::initial_path<boost::filesystem::path>();
}

DataStore::~DataStore() { close(); }

void DataStore::load(const boost::filesystem::path& path) {
  if (boost::filesystem::exists(path) &&
      boost::filesystem::is_directory(path)) {
    _path = path;
  }

  std::ifstream stream;
  stream.exceptions(/*std::ifstream::failbit |*/ std::ifstream::badbit /*|
                    std::ifstream::eofbit*/);
  stream.open((_path / "data").generic_string(),
              std::ios_base::binary | std::ios_base::in);

  if (stream.is_open()) {
    while (stream.peek() != EOF) {
      serialization::BinaryIArchive<std::ifstream> archive(stream);
      store::NewDataPtr newData = std::make_shared<chain::scri::NewData>();
      archive >> newData;
      std::string name = newData->getName();
      _datas.insert(std::make_pair(name, newData));
    }
  }

  stream.close();
}

void DataStore::close() {
  boost::unique_lock<boost::shared_mutex> ulock(_mutex);
  std::ofstream stream;
  stream.exceptions(std::ifstream::failbit | std::ifstream::badbit |
                    std::ifstream::eofbit);
  stream.open((_path / "data").generic_string(),
              std::ios_base::binary | std::ios_base::out);
  for (auto n : _datas) {
    serialization::BinaryOArchive<std::ofstream> archive(stream);
    archive << n.second;
  }
  stream.close();
}

store::NewDataPtr DataStore::query(const std::string& name) {
  boost::shared_lock<boost::shared_mutex> slock(_mutex);
  auto it = _datas.find(name);
  if (it == _datas.end()) {
    return nullptr;
  }
  return it->second;
}

bool DataStore::add(const store::NewDataPtr& dataPtr) {
  if (nullptr != dataPtr) {
    boost::unique_lock<boost::shared_mutex> ulock(_mutex);
    std::string name = dataPtr->getName();
    auto it = _datas.find(name);
    if (it == _datas.end()) {
      _datas.insert(std::make_pair(name, dataPtr));
      return true;
    }
  }
  return false;
}

bool DataStore::add(const chain::DataPtr& dataPtr)
{
	boost::unique_lock<boost::shared_mutex> ulock(_mutex);
	std::string name = dataPtr->getName();
	auto it = datas_.find(name);
	if (it == datas_.end())
	{
		datas_.insert(std::make_pair(name, dataPtr));
		return true;
	}
	return false;
}

}  // namespace store
}  // namespace blockmirror