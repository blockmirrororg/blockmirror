#pragma once

#include <blockmirror/common.h>
#include <blockmirror/serialization/access.h>
#include <boost/algorithm/hex.hpp>

namespace blockmirror {
namespace serialization {

template <typename StreamType>
class JSONOarchive : private boost::noncopyable {
 public:
  using IsJSON = std::true_type;
  using IsSaving = std::true_type;

 private:
  StreamType &_stream;
  size_t _depth;
  size_t _array_size;
  bool _object_begin;
  bool _indent;

  void _tag(const char *tag) {
    if (!_object_begin) _stream << ", ";
    _make_indent();
    _stream << '"' << tag << "\": ";
    _object_begin = false;
  }
  void _make_indent() {
    if (_indent) {
      _stream << '\n' << std::string(2 * _depth, ' ');
    }
  }
  void _begin_object() {
    _stream << "{";
    _object_begin = true;
    ++_depth;
  }
  void _end_object() {
    --_depth;
    _make_indent();
    _stream << "}";
  }
  void _begin_array(size_t s = 0) {
    _array_size = s;
    ++_depth;
    _stream << "[ ";
  }
  void _delimit_array() { _stream << ", "; }
  void _end_array() {
    --_depth;
    if (0 < _array_size) {
      _make_indent();
    }
    _stream << "]";
  }

 public:
  JSONOarchive(StreamType &stream, bool indent = true)
      : _stream(stream),
        _depth(0),
        _array_size(0),
        _object_begin(false),
        _indent(indent) {}

  template <class T>
  JSONOarchive &operator<<(const ::boost::serialization::nvp<T> &t) {
    _tag(t.name());
    return *this << t.const_value();
  }

  template <class T>
  JSONOarchive &operator&(const T &t) {
    return *this << t;
  }

  // Object
  template <class T, typename std::enable_if<!std::is_arithmetic<T>::value &&
                                                 !std::is_enum<T>::value,
                                             int>::type = 0>
  JSONOarchive &operator<<(const T &t) {
    _begin_object();
    access::serialize(*this, const_cast<T &>(t));
    _end_object();
    return *this;
  }
  // Number
  template <class T, typename std::enable_if<std::is_arithmetic<T>::value,
                                             int>::type = 0>
  JSONOarchive &operator<<(const T &t) {
    _stream << +t;
    return *this;
  }
  // Binary
  template <size_t N>
  JSONOarchive &operator<<(const std::array<uint8_t, N> &value) {
    _stream << '"';
    boost::algorithm::hex(value.begin(), value.end(),
                          std::ostream_iterator<char>(_stream));
    _stream << '"';
    return *this;
  }
  JSONOarchive &operator<<(const std::vector<uint8_t> &value) {
    _stream << '"';
    boost::algorithm::hex(value.begin(), value.end(),
                          std::ostream_iterator<char>(_stream));
    _stream << '"';
    return *this;
  }
  // String
  JSONOarchive &operator<<(const std::string &value) {
    _stream << '"' << value << '"';
    return *this;
  }
  // Vector
  template <typename T>
  JSONOarchive &operator<<(const std::vector<T> &arr) {
    _begin_array();
    for (auto &val : arr) {
      *this << val;
      if (&val != &arr.back()) {
        _delimit_array();
      }
    }
    _end_array();
    return *this;
  }
  // boost::variant
  template <typename... T>
  JSONOarchive &operator<<(const boost::variant<T...> &value) {
    boost::apply_visitor(VariantVisitor<JSONOarchive>(*this), value);
    uint8_t type = (uint8_t)value.which();
    *this << BOOST_SERIALIZATION_NVP(type);
    return *this;
  }
};

}  // namespace serialization
}  // namespace blockmirror
