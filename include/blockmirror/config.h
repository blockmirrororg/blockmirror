#pragma once

#include <cstdint>

namespace blockmirror {

constexpr float BP_PERCENT_SIGNER = 80.0f;  // 有80%以上的BP
constexpr uint64_t MINER_AMOUNT = 100000000;    // 给BP的挖矿奖励
constexpr size_t SERIALIZER_MAX_SIZE_T =
    1024 * 1024;  // 序列化读取

constexpr size_t BLOCKSTORE_LIMIT = 256;

constexpr size_t BLOCKSTORE_MAX_FILE = 1024 * 1024 * 1024;

}  // namespace blockmirror
