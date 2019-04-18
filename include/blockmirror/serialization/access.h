#pragma once

#include <blockmirror/common.h>

namespace blockmirror {
namespace serialization {

class access {
 public:
  template <typename Archive, typename T>
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

template <typename S>
struct VariantLoader {
  struct load_null {
    template <typename Archive, class V>
    static void invoke(Archive & /*ar*/, uint32_t /*which*/, V & /*v*/
    ) {}
  };

  struct load_impl {
    template <typename Archive, class V>
    static void invoke(Archive &ar, uint32_t which, V &v) {
      if (which == 0) {
        using head = typename boost::mpl::front<S>::type;
        head value;
        ar >> value;
        v = std::move(value);
      } else {
        using type = typename boost::mpl::pop_front<S>::type;
        VariantLoader<type>::load(ar, which - 1, v);
      }
    }
  };

  template <typename Archive, class V>
  static void load(Archive &ar, uint32_t which, V &v) {
    using typex =
        typename boost::mpl::eval_if<boost::mpl::empty<S>,
                                     boost::mpl::identity<load_null>,
                                     boost::mpl::identity<load_impl> >::type;
    typex::invoke(ar, which, v);
  }
};

template <typename Archive, typename... T>
void VariantLoad(Archive &ar, boost::variant<T...> &v, uint32_t which) {
  using types = typename boost::variant<T...>::types;
  if (which >= boost::mpl::size<types>::value) {
    throw std::runtime_error("VariantLoad bad which");
  }
  VariantLoader<types>::load(ar, which, v);
}

}  // namespace serialization
}  // namespace blockmirror
