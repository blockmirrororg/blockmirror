#include <blockmirror/serialization/binary_iarchive.h>
#include <blockmirror/serialization/binary_oarchive.h>
#include <blockmirror/store/format_store.h>

namespace blockmirror {
namespace store {

FormatStore::FormatStore() {
  _path = boost::filesystem::initial_path<boost::filesystem::path>();
}

FormatStore::~FormatStore() { close(); }

void FormatStore::load(const boost::filesystem::path& path) {
  if (boost::filesystem::exists(path) &&
      boost::filesystem::is_directory(path)) {
    _path = path;
  }

  std::ifstream stream;
  stream.exceptions(/*std::ifstream::failbit | */std::ifstream::badbit /*|
   std::ifstream::eofbit*/);
  stream.open((_path / "format").generic_string(),
              std::ios_base::binary | std::ios_base::in);

  if (stream.is_open()) {
    while (stream.peek() != EOF) {
      serialization::BinaryIArchive<std::ifstream> archive(stream);
      store::NewFormatPtr newFormat =
          std::make_shared<chain::scri::NewFormat>();
      archive >> newFormat;
      std::string name = newFormat->getName();
      _formats.insert(std::make_pair(name, newFormat));
    }
  }

  stream.close();
}

void FormatStore::close() {
  boost::unique_lock<boost::shared_mutex> ulock(_mutex);
  std::ofstream stream;
  stream.exceptions(std::ifstream::failbit | std::ifstream::badbit |
                    std::ifstream::eofbit);
  stream.open((_path / "format").generic_string(),
              std::ios_base::binary | std::ios_base::out);
  for (auto n : _formats) {
    serialization::BinaryOArchive<std::ofstream> archive(stream);
    archive << n.second;
  }
  stream.close();
}

store::NewFormatPtr FormatStore::query(const std::string& name) {
  boost::shared_lock<boost::shared_mutex> slock(_mutex);
  auto it = _formats.find(name);
  if (it == _formats.end()) {
    return nullptr;
  }
  return it->second;
}

bool FormatStore::add(const store::NewFormatPtr& formatPtr) {
  if (nullptr != formatPtr) {
    boost::unique_lock<boost::shared_mutex> ulock(_mutex);
    std::string name = formatPtr->getName();
    auto it = _formats.find(name);
    if (it == _formats.end()) {
      _formats.insert(std::make_pair(name, formatPtr));
      return true;
    }
  }
  return false;
}

}  // namespace store
}  // namespace blockmirror