#pragma once

#include <array>
#include <cstdint>
#include <memory>

namespace blockmirror {

using Hash256 = std::array<uint8_t, 32>;
using Pubkey = std::array<uint8_t, 33>;
using Signature = std::array<uint8_t, 72>;
using Privkey = std::array<uint8_t, 64>;
using Hash256Ptr = std::shared_ptr<Hash256>;

struct SignaturePair {
  Pubkey signer;
  Signature signature;
};

}  // namespace blockmirror

namespace boost {
namespace serialization {

template <typename Archive, typename T, size_t N>
void serialize(Archive &ar, std::array<T, N> &a,
               const unsigned int version) {
  //ar & *static_cast<T (*)[N]>(static_cast<void *>(a.data()));
}

}  // namespace serialization
}  // namespace boost
