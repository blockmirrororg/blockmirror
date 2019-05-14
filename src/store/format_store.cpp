#include <blockmirror/store/format_store.h>
#include <blockmirror/store/store.h>

namespace blockmirror {
namespace store {

FormatStore::FormatStore() : _loaded(false) {
  _path = boost::filesystem::initial_path<boost::filesystem::path>();
}

FormatStore::~FormatStore() {
  if (_loaded) close();
}

void FormatStore::load(const boost::filesystem::path& path) {
  ASSERT(!_loaded);
  _loaded = true;
  _path = path;
  if (boost::filesystem::exists((_path / "format"))) {
    BinaryReader reader;
    reader.open(_path / "format");
    reader >> _formats;
  }
}

void FormatStore::close() {
  ASSERT(_loaded);
  _loaded = false;
  BinaryWritter writter;
  writter.open(_path / "format");
  writter << _formats;
}

const store::NewFormatPtr FormatStore::query(const std::string& name) {
  boost::shared_lock<boost::shared_mutex> lock(_mutex);
  auto it = _formats.find(name);
  if (it == _formats.end()) {
    return nullptr;
  }
  return it->second;
}

bool FormatStore::add(const store::NewFormatPtr& formatPtr) {
  boost::unique_lock<boost::shared_mutex> lock(_mutex);
  auto p = _formats.insert(std::make_pair(formatPtr->getName(), formatPtr));
  return p.second;
}

bool FormatStore::remove(const std::string& name) {
  boost::unique_lock<boost::shared_mutex> lock(_mutex);
  return _formats.erase(name) > 0;
}

}  // namespace store
}  // namespace blockmirror