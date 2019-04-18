#pragma once

#include <blockmirror/common.h>
#include <blockmirror/serialization/access.h>
#include <boost/endian/conversion.hpp>

namespace blockmirror {
namespace serialization {

template <typename StreamType>
class BinaryOArchive : private boost::noncopyable {
 public:
  using IsJSON = std::false_type;
  using IsSaving = std::true_type;

 private:
  StreamType &_stream;

 public:
  BinaryOArchive(StreamType &stream) : _stream(stream) {}

  // Dispatch
  template <typename T>
  BinaryOArchive &operator<<(const ::boost::serialization::nvp<T> &t) {
    return *this << t.const_value();
  }
  template <typename T>
  BinaryOArchive &operator&(const T &t) {
    return *this << t;
  }

  // !Number
  template <typename T, typename std::enable_if<!std::is_arithmetic<T>::value,
                                             int>::type = 0>
  BinaryOArchive &operator<<(const T &t) {
    access::serialize(*this, const_cast<T &>(t));
    return *this;
  }
  // Number
  template <typename T, typename std::enable_if<std::is_arithmetic<T>::value,
                                                int>::type = 0>
  BinaryOArchive &operator<<(T value) {
    if (sizeof(T) > 1 && std::is_integral<T>::value) {
      uint64_t target;
      if (std::is_signed<T>::value) {
        if (value < 0) {
          target = (uint64_t)(-value) << 1;
          target |= 1;
        } else {
          target = (uint64_t)value << 1;
        }
      } else {
        target = (uint64_t)value;
      }
      uint8_t byte;
      while (target > 0x7F) {
        byte = ((uint8_t)(target & 0x7F) | 0x80);
        _stream.write((char *)&byte, sizeof(byte));
        target >>= 7;
      }
      byte = (uint8_t)target;
      _stream.write((char *)&byte, sizeof(byte));
    } else {
      boost::endian::native_to_little_inplace(value);
      _stream.write((char *)&value, sizeof(value));
    }
    return *this;
  }
  // Binary
  template <size_t N>
  BinaryOArchive &operator<<(const std::array<uint8_t, N> &value) {
    _stream.write((char *)&value[0], N);
    return *this;
  }
  BinaryOArchive &operator<<(const std::vector<uint8_t> &value) {
    *this << (uint32_t)value.size();
    if (value.size() > 0) {
      _stream.write((char *)&value[0], value.size());
    }
    return *this;
  }
  // String
  BinaryOArchive &operator<<(const std::string &value) {
    *this << (uint32_t)value.size();
    if (value.size() > 0) {
      _stream.write((char *)value.c_str(), value.size());
    }
    return *this;
  }
  // Vector
  template <typename T>
  BinaryOArchive &operator<<(const std::vector<T> &arr) {
    *this << (uint32_t)arr.size();
    for (auto &val : arr) {
      *this << val;
    }
    return *this;
  }
  // boost::variant
  template <typename... T>
  BinaryOArchive &operator<<(const boost::variant<T...> &value) {
    *this << (uint32_t)value.which();
    boost::apply_visitor(VariantVisitor<BinaryOArchive>(*this), value);
    return *this;
  }
  // shared_ptr
  template <typename T>
  BinaryOArchive &operator<<(const std::shared_ptr<T> &value) {
    if (value.get()) {
      return *this << *value;
    } else {
      return *this << T();
    }
  }
};

}  // namespace serialization
}  // namespace blockmirror
