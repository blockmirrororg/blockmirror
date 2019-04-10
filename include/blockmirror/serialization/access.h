#pragma once

namespace blockmirror {
namespace serialization {

class access {
 public:
  template <class Archive, class T>
  static void serialize(Archive& ar, T& t) {
    t.serialize(ar);
  }
};

}  // namespace serialization
}  // namespace blockmirror
