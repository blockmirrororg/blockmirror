#pragma once

#include <blockmirror/types.h>
#include <unordered_map>
#include <vector>

namespace blockmirror {
namespace chain {

enum class DataType {
  // floating point https://en.wikipedia.org/wiki/IEEE_754
  Float,
  Double,
  // integer
  Uint64,
  Int64,
};

// RPC
class DataValue {
 public:
  uint64_t typeId;
  std::vector<char> buffer;
};

// broadcast
class DataBP : public DataValue {
 public:
  uint64_t bpId;
  Signature signature;  // Verify(bp, sha256(bpId, data, validHeight))
};

/**
 * @brief 保存整个区块的所有BP产生的数据
 * 1. 根据数据类型查找
 * 2. 根据BP类型查找
 * 3. 可以计算有效和结果
 */
class DataStore {
 public:
  std::unordered_map<uint64_t, DataBP> maps;  // typeId
};

class DataResult {
 public:
  std::unordered_map<uint64_t, DataValue> maps;
  // sha256(maps.typeId, buffer)
};

}  // namespace chain
}  // namespace blockmirror