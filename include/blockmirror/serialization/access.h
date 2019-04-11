#pragma once

#include <boost/variant.hpp>

namespace blockmirror {
namespace serialization {

class access {
 public:
  template <class Archive, class T>
  static void serialize(Archive &ar, T &t) {
    t.serialize(ar);
  }
};

template <typename Archive>
class VariantVisitor : public boost::static_visitor<void> {
  Archive &_ar;

 public:
  VariantVisitor(Archive &ar) : _ar(ar){};
  template <typename T>
  void operator()(T &value) const {
    _ar &value;
  }
};

#define SERIALIZE_HASH(ar)                                  \
  if (Archive::IsJSON::value && Archive::IsSaving::value) { \
    auto &hash = getHash();                                 \
    ar &BOOST_SERIALIZATION_NVP(hash);                      \
  }

}  // namespace serialization
}  // namespace blockmirror
