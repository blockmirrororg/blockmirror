#pragma once

#include <cstdint>

namespace blockmirror {

const float BP_PERCENT_SIGNER = 80.0f;             // 有80%以上的BP
const uint64_t MINER_AMOUNT = 100000000;           // 给BP的挖矿奖励
const size_t SERIALIZER_MAX_SIZE_T = 1024 * 1024;  // 序列化读取

const size_t BLOCKSTORE_LIMIT = 256;

const size_t BLOCKSTORE_MAX_FILE = 1024 * 1024 * 1024;

const size_t BLOCK_MAX_ROLLBACK = 120;  // 最多回退120个区块

const uint64_t BLOCK_PER_MS = 1000;

}  // namespace blockmirror
