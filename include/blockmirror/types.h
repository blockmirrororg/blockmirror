#pragma once

#include <array>
#include <cstdint>
#include <memory>

namespace blockmirror {

using Hash256 = std::array<uint8_t, 32>;
using Pubkey = std::array<uint8_t, 33>;
using Signature = std::array<uint8_t, 64>;
using Privkey = std::array<uint8_t, 32>;
using Hash256Ptr = std::shared_ptr<Hash256>;

}  // namespace blockmirror
