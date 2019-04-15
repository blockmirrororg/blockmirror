#pragma once

#include <blockmirror/common.h>
#include <blockmirror/serialization/access.h>
#include <boost/endian/conversion.hpp>

namespace blockmirror {
namespace serialization {

template <typename StreamType>
class BinaryIarchive : private boost::noncopyable {
 public:
  using IsJSON = std::false_type;
  using IsSaving = std::false_type;

 private:
  StreamType &_stream;

 public:
  BinaryIarchive(StreamType &stream) : _stream(stream) {}

  // Dispatch
  template <class T>
  BinaryIarchive &operator>>(::boost::serialization::nvp<T> &t) {
    return *this >> t.value();
  }
  template <class T>
  BinaryIarchive &operator&(T &t) {
    return *this >> t;
  }
  // read size
  size_t readSize() {
    uint32_t val;
    *this >> val;
    if (val > SERIALIZER_MAX_SIZE_T) {
      throw std::runtime_error("iarchive::readSize bad size");
    }
    return (size_t)val;
  }

  // !Number
  template <class T, typename std::enable_if<!std::is_arithmetic<T>::value,
                                             int>::type = 0>
  BinaryIarchive &operator>>(T &t) {
    access::serialize(*this, t);
    return *this;
  }
  // Number
  template <typename T, typename std::enable_if<std::is_arithmetic<T>::value,
                                                int>::type = 0>
  BinaryIarchive &operator>>(T &value) {
    if (sizeof(T) > 1 && std::is_integral<T>::value) {
      uint64_t target = 0;
      uint8_t byte;
      size_t offset = 0;
      do {
        _stream.read((char *)&byte, sizeof(byte));
        target |= ((uint64_t)(byte & 0x7F) << offset);
        offset += 7;
      } while (byte & 0x80);

      if (std::is_signed<T>::value) {
        value = (T)(target >> 1);
        if (target & 1) {
          value = -value;
        }
      } else {
        value = (T)target;
      }
    } else {
      _stream.read((char *)&value, sizeof(value));
      boost::endian::little_to_native_inplace(value);
      return *this;
    }
  }
  // Binary
  template <size_t N>
  BinaryIarchive &operator>>(std::array<uint8_t, N> &value) {
    _stream.read((char *)&value[0], N);
    return *this;
  }
  BinaryIarchive &operator>>(std::vector<uint8_t> &value) {
    size_t size = readSize();
    value.resize(size);
    if (value.size() > 0) {
      _stream.read((char *)&value[0], size);
    }
    return *this;
  }
  // String
  BinaryIarchive &operator>>(std::string &value) {
    size_t size = readSize();
    value.resize(size);
    if (value.size() > 0) {
      _stream.read((char *)value.c_str(), value.size());
    }
    return *this;
  }
  // Vector
  template <typename T>
  BinaryIarchive &operator>>(std::vector<T> &arr) {
    size_t size = readSize();
    arr.resize(size);
    for (auto &val : arr) {
      *this >> val;
    }
    return *this;
  }
  // boost::variant
  template <typename... T>
  BinaryIarchive &operator>>(boost::variant<T...> &value) {
    uint32_t which;
    *this >> which;
    VariantLoad(*this, value, which);
    return *this;
  }
};

}  // namespace serialization
}  // namespace blockmirror
