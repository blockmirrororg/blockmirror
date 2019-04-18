#pragma once

#include <blockmirror/common.h>

namespace blockmirror {
namespace serialization {

class SizeOfOStream {
 private:
  size_t _size;

 public:
  SizeOfOStream() : _size(0) {}

  size_t getSize() const { return _size; }

  void reset() { _size = 0; }

  void write(char *buf, size_t size) { _size += size; }
};

}  // namespace serialization
}  // namespace blockmirror
