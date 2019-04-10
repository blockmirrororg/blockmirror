#pragma once

#include <boost/endian/conversion.hpp>
#include <type_traits>

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

  template <class T, typename std::enable_if<!std::is_arithmetic<T>::value &&
                                                 !std::is_enum<T>::value,
                                             int>::type = 0>
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

  void write(const void *buf, size_t size) { _stream.write((char *)buf, size); }
};

template <typename Archive, typename T,
          typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
BinaryOarchive<Archive> &operator<<(BinaryOarchive<Archive> &archive, T value) {
  boost::endian::native_to_little_inplace(value);
  archive.write(&value, sizeof(value));
  return archive;
}

template <typename Archive, size_t N>
BinaryOarchive<Archive> &operator<<(BinaryOarchive<Archive> &archive,
                                    const std::array<uint8_t, N> &value) {
  archive.write(&value[0], N);
  return archive;
}

template <typename Archive>
BinaryOarchive<Archive> &operator<<(BinaryOarchive<Archive> &archive,
                                    const std::vector<uint8_t> &value) {
  archive << (uint32_t)value.size();
  archive.write(&value[0], value.size());
  return archive;
}

template <typename Archive, typename T,
          typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
BinaryOarchive<Archive> &operator<<(BinaryOarchive<Archive> &archive, T value) {
  return archive << (uint32_t)value;
}

}  // namespace serialization
}  // namespace blockmirror
