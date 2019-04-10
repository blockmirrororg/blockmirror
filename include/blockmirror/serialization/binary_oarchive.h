#pragma once
#include <boost/algorithm/hex.hpp>
#include <boost/endian/conversion.hpp>
#include <iostream>
namespace blockmirror {
namespace serialization {

template <typename StreamType>
class BinaryOarchive {
 private:
  StreamType &_stream;

  BinaryOarchive();
  BinaryOarchive(const BinaryOarchive &o);
  BinaryOarchive(BinaryOarchive &&o);
 public:
  BinaryOarchive(StreamType &stream) : _stream(stream) {}

  template <class T>
  BinaryOarchive &operator<<(const T &t) {
    access::serialize(*this, const_cast<T &>(t));
    return *this;
  }

  template <class T>
  BinaryOarchive &operator<<(const ::boost::serialization::nvp<T> &t) {
    return *this << t.const_value();
  }

  template <class T>
  BinaryOarchive &operator&(const T &t) {
    return *this << t;
  }

  void write(const void *buf, size_t size) {
    _stream.write((char *)buf, size);
  }
};

template <typename T>
BinaryOarchive<T> &operator<<(BinaryOarchive<T> &archive, uint64_t value) {
  boost::endian::native_to_little_inplace(value);
  archive.write(&value, sizeof(value));
  return archive;
}

template <typename T, size_t N>
BinaryOarchive<T> &operator<<(BinaryOarchive<T> &archive,
                              const std::array<uint8_t, N> &value) {
  archive.write(&value[0], N);
  return archive;
}

}  // namespace serialization
}  // namespace blockmirror
