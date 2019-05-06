#pragma once

#include <blockmirror/common.h>
#include <blockmirror/serialization/access.h>
#include <boost/endian/conversion.hpp>

namespace blockmirror {
namespace serialization {

template <typename StreamType>
class BinaryIArchive : private boost::noncopyable {
 public:
  using IsJSON = std::false_type;
  using IsSaving = std::false_type;

 private:
  StreamType &_stream;

 public:
  BinaryIArchive(StreamType &stream) : _stream(stream) {}

  // Dispatch
  template <typename T>
  BinaryIArchive &operator&(const ::boost::serialization::nvp<T> &t) {
    return *this >> t.value();
  }
  template <typename T>
  BinaryIArchive &operator&(T &t) {
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
  template <typename T, typename std::enable_if<!std::is_arithmetic<T>::value,
                                                int>::type = 0>
  BinaryIArchive &operator>>(T &t) {
    access::serialize(*this, t);
    return *this;
  }
  // Number
  template <typename T, typename std::enable_if<std::is_arithmetic<T>::value,
                                                int>::type = 0>
  BinaryIArchive &operator>>(T &value) {
    if (sizeof(T) > 1 && std::is_integral<T>::value) {
      uint64_t target = 0;
      uint8_t byte;
      size_t offset = 0;
      int counter = 0;
      do {
        if (++counter > 10) {
          throw std::runtime_error("BinaryIArchive bad integer");
        }
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
    }
    return *this;
  }
  // Binary
  template <size_t N>
  BinaryIArchive &operator>>(std::array<uint8_t, N> &value) {
    _stream.read((char *)&value[0], N);
    return *this;
  }
  BinaryIArchive &operator>>(std::vector<uint8_t> &value) {
    size_t size = readSize();
    value.resize(size);
    if (value.size() > 0) {
      _stream.read((char *)&value[0], size);
    }
    return *this;
  }
  // String
  BinaryIArchive &operator>>(std::string &value) {
    size_t size = readSize();
    value.resize(size);
    if (value.size() > 0) {
      _stream.read((char *)value.c_str(), value.size());
    }
    return *this;
  }
  // Vector
  template <typename T>
  BinaryIArchive &operator>>(std::vector<T> &arr) {
    size_t size = readSize();
    arr.resize(size);
    for (auto &val : arr) {
      *this >> val;
    }
    return *this;
  }
  // boost::variant
  template <typename... T>
  BinaryIArchive &operator>>(boost::variant<T...> &value) {
    uint32_t which;
    *this >> which;
    VariantLoad(*this, value, which);
    return *this;
  }
  // shared_ptr
  template <typename T>
  BinaryIArchive &operator>>(std::shared_ptr<T> &value) {
    value = std::make_shared<T>();
    *this >> *value;
    return *this;
  }
  // unordered_map
  template <typename Key, typename T, typename Hash, typename KeyEqual>
  BinaryIArchive &operator>>(std::unordered_map<Key, T, Hash, KeyEqual> &value) {
    size_t size = readSize();
    for (size_t i = 0; i < size; i++) {
      Key key;
      T val;
      *this >> key >> val;
      value.insert(std::make_pair(key, val));
    }
    return *this;
  }
};

}  // namespace serialization
}  // namespace blockmirror
