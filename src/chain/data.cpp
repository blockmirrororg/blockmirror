#include <blockmirror/chain/data.h>
#include <blockmirror/serialization/binary_oarchive.h>
#include <blockmirror/serialization/hash_ostream.h>

namespace blockmirror {
namespace chain {

Data::Data(const std::string &n, const std::vector<uint8_t> &d)
    : name(n), data(d) {}
Data::Data(std::string &&n, const std::vector<uint8_t> &d) : name(n), data(d) {}
Data::Data(const std::string &n, std::vector<uint8_t> &&d) : name(n), data(d) {}
Data::Data(std::string &&n, std::vector<uint8_t> &&d) : name(n), data(d) {}
Data::Data(const Data &o) : name(o.name), data(o.data) {}
Data::Data(Data &&o) : name(o.name), data(o.data) {}

void Data::getHash(const Pubkey &bp, uint64_t height, Hash256 &hash) const {
  serialization::HashOStream oss;
  serialization::BinaryOarchive<decltype(oss)> oa(oss);
  oa << bp << height << *this;
  oss.getHash(hash);
}

void DataSigned::sign(const Privkey &priv, uint64_t height,
                      const crypto::ECCContext &ecc) {
  Pubkey pub;
  Hash256 hash;
  ecc.computePub(priv, pub);
  getHash(pub, height, hash);
  ecc.sign(priv, hash, signature);
}
bool DataSigned::verify(const Pubkey &pub, uint64_t height,
                        const crypto::ECCContext &ecc) {
  Hash256 hash;
  getHash(pub, height, hash);
  return ecc.verify(pub, hash, signature);
}
bool DataSigned::verify(const Privkey &priv, uint64_t height,
                        const crypto::ECCContext &ecc) {
  Pubkey pub;
  ecc.computePub(priv, pub);
  return verify(pub, height, ecc);
}

DataBP::DataBP(const Pubkey &b) : bp(b) {}
DataBP::DataBP(const DataBP &o) : bp(o.bp), datas(o.datas) {}
DataBP::DataBP(DataBP &&o) : bp(o.bp), datas(o.datas) {}

}  // namespace chain
}  // namespace blockmirror
