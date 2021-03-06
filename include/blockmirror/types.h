#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include <boost/algorithm/hex.hpp>
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

template <size_t N>
void fromHex(std::array<uint8_t, N>& s, const std::string& hexStr) {
  if (hexStr.size() != N * 2) {
    throw std::runtime_error("bad hexstr length");
  }
  boost::algorithm::unhex(hexStr, s.begin());
}

template <typename Container>
std::string toHex(const Container& s) {
  return boost::algorithm::hex(s);
}

struct Hasher {
  template <size_t N>
  size_t operator()(const std::array<uint8_t, N>& s) const {
    BOOST_STATIC_ASSERT(N > sizeof(size_t) + 1);
    size_t size;
    memcpy(&size, &s[1], sizeof(size));
    boost::endian::native_to_little_inplace(size);
    return size;
  }
  template <size_t N>
  size_t operator()(const std::shared_ptr<std::array<uint8_t, N>>& s) const {
    if (!s) return 0;
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

struct Less {
  template <size_t N>
  bool operator()(const std::array<uint8_t, N>& x,
                  const std::array<uint8_t, N>& y) const {
    return memcmp(x.data(), y.data(), N) < 0;
  }
  template <size_t N>
  bool operator()(const std::shared_ptr<std::array<uint8_t, N>>& x,
                  const std::shared_ptr<std::array<uint8_t, N>>& y) const {
    if (!x) return true;
    if (!y) return false;
    return operator()(*x, *y);
  }
};

}  // namespace blockmirror
