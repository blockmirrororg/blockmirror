#pragma once

#include <blockmirror/common.h>
#include <blockmirror/crypto/ecc.h>
#include <blockmirror/serialization/access.h>

namespace blockmirror {
namespace chain {

// rpc
class Data {
 protected:
  friend class blockmirror::serialization::access;
  template <class Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(name) & BOOST_SERIALIZATION_NVP(data);
  }

 protected:
  std::string name;
  std::vector<uint8_t> data;

 public:
  Data(const std::string &n, const std::vector<uint8_t> &d);
  Data(std::string &&n, const std::vector<uint8_t> &d);
  Data(const std::string &n, std::vector<uint8_t> &&d);
  Data(std::string &&n, std::vector<uint8_t> &&d);
  Data(const Data &o);
  Data(Data &&o);
  Data() = default;

  const std::string &getName() { return name; }
  const std::vector<uint8_t> &getData() { return data; }
  void getHash(const Pubkey &bp, uint64_t height, Hash256 &hash) const;
};
using DataPtr = std::shared_ptr<Data>;

// broadcast
class DataSigned : public Data {
 protected:
  friend class blockmirror::serialization::access;
  template <class Archive>
  void serialize(Archive &ar) {
    Data::serialize(ar);
    ar &BOOST_SERIALIZATION_NVP(signature);
  }

 protected:
  Signature signature;

 public:
  using Data::Data;

  const Signature &getSignature() { return signature; }
  void sign(const Privkey &priv, uint64_t height,
            const crypto::ECCContext &ecc = crypto::ECC);
  bool verify(const Pubkey &pub, uint64_t height,
              const crypto::ECCContext &ecc = crypto::ECC);
  bool verify(const Privkey &priv, uint64_t height,
              const crypto::ECCContext &ecc = crypto::ECC);
};
using DataSignedPtr = std::shared_ptr<DataSigned>;

class DataBP {
 protected:
  friend class blockmirror::serialization::access;
  template <class Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(bp) & BOOST_SERIALIZATION_NVP(datas);
  }

 protected:
  Pubkey bp;
  std::vector<DataSignedPtr> datas;

 public:
  DataBP(const Pubkey &b);
  DataBP(const DataBP &o);
  DataBP(DataBP &&o);
  DataBP() = default;

  const Pubkey &getBP() const { return bp; }
  const std::vector<DataSignedPtr> &getDatas() const { return datas; }
  void addData(const DataSignedPtr &data) { datas.push_back(data); }
};
using DataBPPtr = std::shared_ptr<DataBP>;

}  // namespace chain
}  // namespace blockmirror