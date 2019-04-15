#pragma once

#include <cstdint>

namespace blockmirror {

constexpr float BP_PERCENT_SIGNER = 80.0f;  // 有80%以上的BP
constexpr uint64_t MINER_AMOUNT = 10000;    // 给BP的挖矿奖励
constexpr size_t SERIALIZER_MAX_SIZE_T =
    1024 * 1024;  // 序列化读取

}  // namespace blockmirror
