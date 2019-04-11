#pragma once

#include <blockmirror/serialization/access.h>
#include <blockmirror/types.h>
#include <boost/serialization/nvp.hpp>
#include <vector>

namespace blockmirror {
namespace chain {

// RPC
class DataValue {
 protected:
  friend class blockmirror::serialization::access;
  template <class Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(typeId) & BOOST_SERIALIZATION_NVP(data);
  }
 public:
  uint64_t typeId;
  std::vector<char> data;
};

// BP
class DataBP : public DataValue {
 protected:
  friend class blockmirror::serialization::access;
  template <class Archive>
  void serialize(Archive &ar) {
    DataValue::serialize(ar);
    ar &BOOST_SERIALIZATION_NVP(bpId) & BOOST_SERIALIZATION_NVP(signature);
  }
 public:
  uint64_t bpId;
  Signature signature;  // Verify(bp, sha256(bpId, data, validHeight))
};

}  // namespace chain
}  // namespace blockmirror