#pragma once

#include <blockmirror/chain/context.h>
#include <blockmirror/common.h>
#include <blockmirror/serialization/access.h>
#include <boost/algorithm/hex.hpp>

namespace blockmirror {
namespace serialization {

template <typename StreamType>
class BSONOArchive : private boost::noncopyable {
 public:
  using IsBSON = std::true_type;
  using IsSaving = std::true_type;

 private:
  StreamType &_stream;
  chain::Context &_context;
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
  BSONOArchive(StreamType &stream, chain::Context &context, bool indent = true)
      : _stream(stream),
        _context(context),
        _depth(0),
        _array_size(0),
        _object_begin(false),
        _indent(indent) {}

  template <typename T>
  BSONOArchive &operator<<(const ::boost::serialization::nvp<T> &t) {
    _tag(t.name());
    return *this << t.const_value();
  }

  template <typename T>
  BSONOArchive &operator&(const T &t) {
    return *this << t;
  }

  // Object
  template <typename T, typename std::enable_if<!std::is_arithmetic<T>::value &&
                                                    !std::is_enum<T>::value,
                                                int>::type = 0>
  BSONOArchive &operator<<(const T &t) {
    _begin_object();
    access::serialize(*this, const_cast<T &>(t));
    _end_object();
    return *this;
  }
  // Number
  template <typename T, typename std::enable_if<std::is_arithmetic<T>::value,
                                                int>::type = 0>
  BSONOArchive &operator<<(const T &t) {
    _stream << +t;
    return *this;
  }
  // Binary
  template <size_t N>
  BSONOArchive &operator<<(const std::array<uint8_t, N> &value) {
    _stream << '"';
    boost::algorithm::hex(value.begin(), value.end(),
                          std::ostream_iterator<char>(_stream));
    _stream << '"';
    return *this;
  }
  BSONOArchive &operator<<(const std::vector<uint8_t> &value) {
    _stream << '"';
    boost::algorithm::hex(value.begin(), value.end(),
                          std::ostream_iterator<char>(_stream));
    _stream << '"';
    return *this;
  }
  // String
  BSONOArchive &operator<<(const std::string &value) {
    _stream << '"' << value << '"';
    return *this;
  }
  // Vector
  template <typename T>
  BSONOArchive &operator<<(const std::vector<T> &arr) {
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

  // Vector<chain::DataSignedPtr>
  BSONOArchive &operator<<(const std::vector<chain::DataSignedPtr> &arr) {
    _begin_array();
    for (auto &val : arr) {
      std::string name = val->getName();
      _stream << "{" << '"' << "name" << '"' << ": ";
      _stream << '"' << name << '"' << ", ";
      _stream << '"' << "data" << '"' << ": " << '"';
      store::FormatStore &store = _context.getFormatStore();
      store::NewFormatPtr n = store.query(name);
      if (n) {
        const std::vector<uint8_t> v = n->getDataFormat();
        for (const auto &i : v) {
          *this << i;
        }
      }
      _stream << '"' << ", ";
      _stream << '"' << "signature" << '"' << ": " << '"';
      boost::algorithm::hex(val->getSignature().begin(),
                            val->getSignature().end(),
                            std::ostream_iterator<char>(_stream));
      _stream << '"' << "}";

      if (&val != &arr.back()) {
        _delimit_array();
      }
    }
    _end_array();
    return *this;
  }

  // boost::variant
  template <typename... T>
  BSONOArchive &operator<<(const boost::variant<T...> &value) {
    _begin_object();
    uint32_t type = (uint32_t)value.which();
    *this << BOOST_SERIALIZATION_NVP(type);
    _tag("value");
    boost::apply_visitor(VariantVisitor<BSONOArchive>(*this), value);
    _end_object();
    return *this;
  }
  // shared_ptr
  template <typename T>
  BSONOArchive &operator<<(const std::shared_ptr<T> &value) {
    if (value.get()) {
      return *this << *value;
    } else {
      return *this << T();
    }
  }
};

}  // namespace serialization
}  // namespace blockmirror