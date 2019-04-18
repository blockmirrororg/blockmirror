#pragma once

#include <blockmirror/chain/block.h>

namespace blockmirror {
namespace chain {

/**
 * 1. 空闲时新来的数据和交易都应用到pending上
 * 2. 打包区块时直接将pending设置完后签名
 * 3. 收到新区块时
 *     1. 如果 需要回滚 则回退到共同父区块 回退过程会将交易重新放入交易池或者修改交易未未确认状态
 *     2. 开始应用区块 应用区块时会影响交易池
 *     4. 应用完成后重建pending 将未过期以及未确认的交易包含进来
 * 
 * 回滚机制最长链原则
 * 如果新区块的长度超过当前区块的长度，则回滚
 */
class Context {
 protected:
  BlockPtr _pending; // 正在打包的区块
  BlockPtr _head; // 最后应用的区块

};

}  // namespace chain
}  // namespace blockmirror
