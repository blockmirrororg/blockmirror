#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include <boost/endian/conversion.hpp>

namespace blockmirror {

using Hash256 = std::array<uint8_t, 32>;
using Pubkey = std::array<uint8_t, 33>;
using Signature = std::array<uint8_t, 64>;
using Privkey = std::array<uint8_t, 32>;
using Hash256Ptr = std::shared_ptr<Hash256>;
using PubkeyPtr = std::shared_ptr<Pubkey>;
using SignaturePtr = std::shared_ptr<Signature>;
using PrivkeyPtr = std::shared_ptr<Privkey>;

struct Hasher {
  template <size_t N>
  size_t operator()(const std::array<uint8_t, N>& s) const {
    BOOST_STATIC_ASSERT(N > sizeof(size_t) + 1);
    size_t size = *(size_t*)&s[1];
    boost::endian::native_to_little_inplace(size);
    return size;
  }
  template <size_t N>
  size_t operator()(const std::shared_ptr<std::array<uint8_t, N>>& s) const {
    return operator()(*s);
  }
};

struct EqualTo {
  template <size_t N>
  bool operator()(const std::array<uint8_t, N>& x,
                  const std::array<uint8_t, N>& y) const {
    return memcmp(x.data(), y.data(), N) == 0;
  }
  template <size_t N>
  bool operator()(const std::shared_ptr<std::array<uint8_t, N>>& x,
                  const std::shared_ptr<std::array<uint8_t, N>>& y) const {
    if (!x) return !y;
    if (!y) return false;
    return *x == *y;
  }
};

}  // namespace blockmirror
