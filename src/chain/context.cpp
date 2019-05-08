#include <blockmirror/chain/context.h>

namespace blockmirror {
namespace chain {

Context::Context() {}

Context::~Context() {}

void Context::_apply(const TransactionSignedPtr &trx) {

}


bool Context::check(const chain::BlockHeaderSignedPtr &block) {
  if (!block) return false;

  if (_head) {
    // 最大可回滚
    if (block->getHeight() + BLOCK_MAX_ROLLBACK < _head->getHeight()) {
      return false;
    }
  }

  // FIXME: 这里有隐含的问题，具体情况以后考虑
  if (!_bps.contains(block->getProducer())) {
    return false;
  }

  if (!block->verify()) {
    return false;
  }

  return true;
}

}  // namespace chain
}  // namespace blockmirror