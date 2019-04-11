#pragma once

#include <boost/endian/conversion.hpp>
#include <boost/noncopyable.hpp>
#include <type_traits>

namespace blockmirror {
namespace serialization {

template <typename StreamType>
class BinaryOarchive : private boost::noncopyable {
 public:
  using IsJSON = std::false_type;
  using IsSaving = std::true_type;

 private:
  StreamType &_stream;

 public:
  BinaryOarchive(StreamType &stream) : _stream(stream) {}

  template <class T>
  BinaryOarchive &operator<<(const ::boost::serialization::nvp<T> &t) {
    return *this << t.const_value();
  }

  template <class T>
  BinaryOarchive &operator&(const T &t) {
    return *this << t;
  }

  // Object
  template <class T, typename std::enable_if<!std::is_arithmetic<T>::value &&
                                                 !std::is_enum<T>::value,
                                             int>::type = 0>
  BinaryOarchive &operator<<(const T &t) {
    access::serialize(*this, const_cast<T &>(t));
    return *this;
  }
  // Number
  template <typename T, typename std::enable_if<std::is_arithmetic<T>::value,
                                                int>::type = 0>
  BinaryOarchive &operator<<(T value) {
    boost::endian::native_to_little_inplace(value);
    _stream.write((char *)&value, sizeof(value));
    return *this;
  }
  // Enum
  template <typename T,
            typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
  BinaryOarchive &operator<<(T value) {
    return *this << (uint32_t)value;
  }
  // Binary
  template <size_t N>
  BinaryOarchive &operator<<(const std::array<uint8_t, N> &value) {
    _stream.write((char *)&value[0], N);
    return *this;
  }
  BinaryOarchive &operator<<(const std::vector<uint8_t> &value) {
    *this << (uint32_t)value.size();
    if (value.size() > 0) {
      _stream.write((char *)&value[0], value.size());
    }
    return *this;
  }
  // String
  BinaryOarchive &operator<<(const std::string &value) {
    *this << (uint32_t)value.size();
    _stream << value;
    return *this;
  }
  // Vector
  template <typename T>
  BinaryOarchive &operator<<(const std::vector<T> &arr) {
    *this << (uint32_t)arr.size();
    for (auto &val : arr) {
      *this << val;
    }
    return *this;
  }
  // boost::variant
  template <typename... T>
  BinaryOarchive &operator<<(const boost::variant<T...> &value) {
    return *this;
  }
};

}  // namespace serialization
}  // namespace blockmirror
