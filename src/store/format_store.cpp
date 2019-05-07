#include <blockmirror/store/format_store.h>
#include <blockmirror/store/store.h>

namespace blockmirror {
namespace store {

FormatStore::FormatStore() {
  _path = boost::filesystem::initial_path<boost::filesystem::path>();
}

FormatStore::~FormatStore() { close(); }

void FormatStore::load(const boost::filesystem::path& path) {
  _path = path;
  if (boost::filesystem::exists((_path / "format"))) {
    BinaryReader reader;
    reader.open(_path / "format");
    reader >> _formats;
  }
}

void FormatStore::close() {
  BinaryWritter writter;
  writter.open(_path / "format");
  writter << _formats;
}

const store::NewFormatPtr FormatStore::query(const std::string& name) {
  boost::shared_lock<boost::shared_mutex> slock(_mutex);
  auto it = _formats.find(name);
  if (it == _formats.end()) {
    return nullptr;
  }
  return it->second;
}

bool FormatStore::add(const store::NewFormatPtr& formatPtr) {
  boost::unique_lock<boost::shared_mutex> ulock(_mutex);
  auto p = _formats.insert(std::make_pair(formatPtr->getName(), formatPtr));
  return p.second;
}

}  // namespace store
}  // namespace blockmirror