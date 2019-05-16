#pragma once

#include <blockmirror/serialization/binary_iarchive.h>
#include <blockmirror/serialization/binary_oarchive.h>

namespace blockmirror {
namespace store {

class BinaryWritter {
 protected:
  std::ofstream _stream;
  serialization::BinaryOArchive<std::ofstream> _archive;

 public:
  BinaryWritter() : _archive(_stream) {}

  void open(const boost::filesystem::path &path,
            std::ios_base::openmode mode = std::ios_base::binary |
                                           std::ios_base::out) {
    _stream.exceptions(std::fstream::failbit | std::fstream::badbit |
                       std::fstream::eofbit);
    _stream.open(path.generic_string(), mode);
  }

  std::ofstream::pos_type tellp() { return _stream.tellp(); }

  void seekEnd() { _stream.seekp(0, std::ios::end); }

  void close() { _stream.close(); }

  template <typename T>
  BinaryWritter &operator<<(const T &t) {
    _archive << t;
    return *this;
  }
};

class BinaryReader {
 protected:
  std::ifstream _stream;
  serialization::BinaryIArchive<std::ifstream> _archive;

 public:
  BinaryReader() : _archive(_stream) {}

  void open(const boost::filesystem::path &path,
            std::ios_base::openmode mode = std::ios_base::binary |
                                           std::ios_base::in) {
    _stream.exceptions(std::fstream::failbit | std::fstream::badbit |
                       std::fstream::eofbit);
    _stream.open(path.generic_string(), mode);
  }

  void seekg(std::ifstream::pos_type pos) { _stream.seekg(pos); }

  template <typename T>
  BinaryReader &operator>>(T &t) {
    _archive >> t;
    return *this;
  }
};

}  // namespace store
}  // namespace blockmirror