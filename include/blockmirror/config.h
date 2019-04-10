#pragma once

#include <cstdint>

namespace blockmirror {

constexpr float VOTEBP_PERCENT_SIGNER = 80.0f;  // 推荐出新BP必须有80%以上的BP
constexpr float EXITBP_PERCENT_SIGNER = 80.0f;  // 推荐出新BP必须有80%以上的BP
constexpr float VOTENEWDATA_PERCENT_SIGNER =
    80.0f;  // 增加新的数据必须有80以上的BP
constexpr uint64_t MINER_AMOUNT = 10000;  // 给BP的挖矿奖励

}  // namespace blockmirror
