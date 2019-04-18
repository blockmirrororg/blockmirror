#pragma once

#include <blockmirror/common.h>
#include <blockmirror/serialization/access.h>
#include <boost/endian/conversion.hpp>
#include <boost/property_tree/ptree.hpp>

namespace blockmirror {
namespace serialization {

class PTreeIArchive : private boost::noncopyable {
 public:
  using IsJSON = std::true_type;
  using IsSaving = std::false_type;

 private:
  boost::property_tree::ptree &_tree;

 public:
  PTreeIArchive(boost::property_tree::ptree &tree) : _tree(tree) {}

  // Dispatch
  template <typename T>
  PTreeIArchive &operator&(const ::boost::serialization::nvp<T> &t) {
    PTreeIArchive child(_tree.get_child(t.name()));
    child >> t.value();
    return *this;
  }
  template <typename T>
  PTreeIArchive &operator&(T &t) {
    return *this >> t;
  }
  void checkSize(uint32_t value) {
    if (value > SERIALIZER_MAX_SIZE_T) {
      throw std::runtime_error("iarchive::checkSize bad size");
    }
  }

  // !Number
  template <typename T, typename std::enable_if<!std::is_arithmetic<T>::value,
                                             int>::type = 0>
  PTreeIArchive &operator>>(T &t) {
    access::serialize(*this, t);
    return *this;
  }
  // Number
  template <typename T, typename std::enable_if<std::is_arithmetic<T>::value,
                                                int>::type = 0>
  PTreeIArchive &operator>>(T &value) {
    value = _tree.get<T>("");
    return *this;
  }
  // Binary
  template <size_t N>
  PTreeIArchive &operator>>(std::array<uint8_t, N> &value) {
    const std::string &str = _tree.get<std::string>("");
    if (str.length() / 2 != N) {
      throw std::runtime_error("bad std array length");
    }
    boost::algorithm::unhex(str, value.begin());
    return *this;
  }
  PTreeIArchive &operator>>(std::vector<uint8_t> &value) {
    const std::string &str = _tree.get<std::string>("");
    checkSize(str.length() / 2);
    value.reserve(str.length() / 2);
    boost::algorithm::unhex(str, std::back_inserter(value));
    return *this;
  }
  // String
  PTreeIArchive &operator>>(std::string &value) {
    value = _tree.get<std::string>("");
    return *this;
  }
  // Vector
  template <typename T>
  PTreeIArchive &operator>>(std::vector<T> &arr) {
    for (auto& item : _tree.get_child("")) {
      PTreeIArchive child(item.second);
      arr.push_back(T());
      child >> arr.back();
    }
    return *this;
  }
  // boost::variant
  template <typename... T>
  PTreeIArchive &operator>>(boost::variant<T...> &value) {
    uint32_t type = _tree.get<uint32_t>("type");
    auto tree = _tree.get_child("value");
    PTreeIArchive child(tree);
    VariantLoad(child, value, type);
    return *this;
  }
  // shared_ptr
  template <typename T>
  PTreeIArchive &operator>>(std::shared_ptr<T> &value) {
    value = std::make_shared<T>();
    *this >> *value;
    return *this;
  }
};

}  // namespace serialization
}  // namespace blockmirror
