#pragma once

#include <blockmirror/chain/context.h>
#include <blockmirror/common.h>
#include <blockmirror/serialization/access.h>

#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/types.hpp>

namespace blockmirror {
namespace serialization {

template <typename T, typename Archive>
class has_serialze {
 private:
  typedef char YesType[1];
  typedef char NoType[2];

  template <typename C>
  static YesType &test(decltype(&C::template serialize<Archive>));
  template <typename C>
  static NoType &test(...);

 public:
  enum { value = sizeof(test<T>(0)) == sizeof(YesType) };
};

template <typename Archive>
class BSONVisitor : public boost::static_visitor<void> {
  Archive &_ar;

 public:
  BSONVisitor(Archive &ar) : _ar(ar){};

  template <typename T>
  typename std::enable_if<has_serialze<T, Archive>::value, void>::type
  operator()(T &value) const;
  template <typename T>
  typename std::enable_if<!has_serialze<T, Archive>::value, void>::type
  operator()(T &value) const {
    _ar << value;
  }
};

template <typename DOCUMENT>
class BSONOArchive : private boost::noncopyable {
 public:
  using IsJSON = std::true_type;  // 假装是JSON
  using IsSaving = std::true_type;

#define BSONCXX_ENUM(name, val)                                     \
  BSONOArchive &operator<<(const bsoncxx::types::b_##name &value) { \
    _stream << value;                                               \
    return *this;                                                   \
  }
#include <bsoncxx/enums/type.hpp>
#undef BSONCXX_ENUM

 private:
  DOCUMENT &_stream;
  chain::Context &_context;

 public:
  inline DOCUMENT &doc() { return _stream; }
  inline chain::Context &ctx() { return _context; }

  BSONOArchive(chain::Context &context, DOCUMENT &doc)
      : _context(context), _stream(doc) {}

  template <typename T>
  BSONOArchive &operator<<(const ::boost::serialization::nvp<T> &t) {
    auto valueDoc = (_stream << std::string(t.name()));
    BSONOArchive<decltype(valueDoc)> value(_context, valueDoc);
    value << t.const_value();
    return *this;
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
    bsoncxx::builder::stream::document doc;
    BSONOArchive<decltype(doc)> archiveValue(ctx(), doc);
    access::serialize(archiveValue, const_cast<T &>(t));
    _stream << bsoncxx::builder::concatenate(doc.view());
    return *this;
  }

  // chain::BlockPtr
  template <
      typename std::enable_if<!std::is_arithmetic<chain::BlockPtr>::value &&
                                  !std::is_enum<chain::BlockPtr>::value,
                              int>::type = 0>
  BSONOArchive &operator<<(const chain::BlockPtr &t) {
    bsoncxx::builder::stream::document doc;
    doc << "hash"
        << boost::algorithm::hex(
               std::string(t->getHash().begin(), t->getHash().end()));
    BSONOArchive<decltype(doc)> archiveValue(ctx(), doc);
    access::serialize(archiveValue, *t);
    _stream << bsoncxx::builder::concatenate(doc.view());
    return *this;
  }

  // Number
  template <typename T,
            typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  BSONOArchive &operator<<(const T &t) {
    _stream << bsoncxx::types::b_int64{(int64_t)t};
    return *this;
  }
  template <typename T, typename std::enable_if<
                            std::is_floating_point<T>::value, int>::type = 0>
  BSONOArchive &operator<<(const T &t) {
    _stream << bsoncxx::types::b_double{(double)t};
    return *this;
  }
  // Binary
  template <size_t N>
  BSONOArchive &operator<<(const std::array<uint8_t, N> &value) {
    _stream << boost::algorithm::hex(std::string(value.begin(), value.end()));
    return *this;
  }
  BSONOArchive &operator<<(const std::vector<uint8_t> &value) {
    _stream << boost::algorithm::hex(std::string(value.begin(), value.end()));
    return *this;
  }
  // String
  BSONOArchive &operator<<(const std::string &value) {
    _stream << value;
    return *this;
  }
  // Vector
  template <typename T>
  BSONOArchive &operator<<(const std::vector<T> &arr) {
    auto arrDoc = (_stream << bsoncxx::builder::stream::open_array);
    BSONOArchive<decltype(arrDoc)> archive(_context, arrDoc);
    for (auto &val : arr) {
      archive << val;
    }
    arrDoc << bsoncxx::builder::stream::close_array;
    return *this;
  }
  // boost::variant
  template <typename... T>
  BSONOArchive &operator<<(const boost::variant<T...> &value) {
    auto objDoc = (_stream << bsoncxx::builder::stream::open_document);

    BSONOArchive<decltype(objDoc)> archive(_context, objDoc);
    uint32_t type = (uint32_t)value.which();
    archive << BOOST_SERIALIZATION_NVP(type);

    auto valueDoc = (objDoc << std::string("value"));

    BSONOArchive<decltype(valueDoc)> archiveValue(_context, valueDoc);
    boost::apply_visitor(BSONVisitor<decltype(archiveValue)>(archiveValue),
                         value);

    objDoc << bsoncxx::builder::stream::close_document;
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

  // Vector<chain::DataSignedPtr>
  BSONOArchive &operator<<(const std::vector<chain::DataSignedPtr> &arr) {
    auto arrDoc = (_stream << bsoncxx::builder::stream::open_array);
    for (auto &val : arr) {
      std::ostringstream oss;
      std::string name = val->getName();
      store::DataStore &ds = _context.getDataStore();
      store::NewDataPtr nd = ds.query(name);
      if (nd) {
        store::FormatStore &fs = _context.getFormatStore();
        store::NewFormatPtr nf = fs.query(nd->getFormat());
        if (nf) {
          const std::vector<uint8_t> data = val->getData();
          const std::vector<uint8_t> dataFormat = nf->getDataFormat();
          size_t formatSize = dataFormat.size();
          size_t index = 0;
          for (size_t i = 0; i < formatSize; i++) {
            switch (dataFormat[i]) {
              case chain::scri::NewFormat::TYPE_FLOAT:
                float fl;
                memcpy(&fl, &data[index], sizeof(float));
                oss << fl;
                index += sizeof(float);
                break;
              case chain::scri::NewFormat::TYPE_DOUBLE:
                double dou;
                memcpy(&dou, &data[index], sizeof(double));
                oss << dou;
                index += sizeof(double);
                break;
              case chain::scri::NewFormat::TYPE_UINT:
                uint32_t uin;
                memcpy(&uin, &data[index], sizeof(uint32_t));
                oss << uin;
                index += sizeof(uint32_t);
                break;
              case chain::scri::NewFormat::TYPE_INT:
                int in;
                memcpy(&in, &data[index], sizeof(int));
                oss << in;
                index += sizeof(int);
                break;
              default:;
            }
            if ((i + 1) != formatSize) {
              oss << ",";
            }
          }
        }
      }
      arrDoc =
          arrDoc << bsoncxx::builder::stream::open_document << "name" << name
                 << "data" << oss.str() << "signature"
                 << boost::algorithm::hex(std::string(
                        val->getSignature().begin(), val->getSignature().end()))
                 << bsoncxx::builder::stream::close_document;
    }
    arrDoc << bsoncxx::builder::stream::close_array;
    return *this;
  }
};

template <typename Archive>
template <typename T>
typename std::enable_if<has_serialze<T, Archive>::value, void>::type
BSONVisitor<Archive>::operator()(T &value) const {
  bsoncxx::builder::stream::document doc;
  BSONOArchive<decltype(doc)> archiveValue(_ar.ctx(), doc);
  archiveValue << value;
  _ar.doc() << bsoncxx::builder::concatenate(doc.view());
}

}  // namespace serialization
}  // namespace blockmirror