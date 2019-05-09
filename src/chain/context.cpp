#include <blockmirror/chain/context.h>

namespace blockmirror {
namespace chain {

Context::Context() {}

Context::~Context() {}

void Context::load() {
  _account.load(".");
  _block.load(".");
  _bps.load(".");
  _data.load(".");
  _format.load(".");
  _transaction.load(".");
}
void Context::close() {
  _account.close();
  _block.close();
  _bps.close();
  _data.close();
  _format.close();
  _transaction.close();
}

void Context::_apply(const TransactionSignedPtr& trx) {
  if (_transaction.add(trx)) {
    const std::vector<SignaturePair> v = trx->getSignatures();
    if (!v.empty()) {
      Pubkey signer = v[0].signer;
      Script script = trx->getScript();
      boost::apply_visitor(StoreVisitor(*this, signer, 0), script);
    }
  }
}

void Context::_rollback(const chain::TransactionSignedPtr& trx) {
  if (_transaction.remove(trx)) {
    const std::vector<SignaturePair> v = trx->getSignatures();
    if (!v.empty()) {
      Pubkey signer = v[0].signer;
      Script script = trx->getScript();
      boost::apply_visitor(StoreVisitor(*this, signer, 1), script);
    }
  }
}

void Context::apply(const chain::BlockPtr& block) {
  const std::vector<TransactionSignedPtr> v = block->getTransactions();
  for (auto it = v.begin(); it != v.end(); ++it) {
    _apply(*it);
  }
}

void Context::rollback(const chain::BlockPtr& block) {
  const std::vector<TransactionSignedPtr> v = block->getTransactions();
  for (auto it = v.rbegin(); it != v.rend(); ++it) {
    _rollback(*it);
  }
}

bool Context::check(const chain::TransactionSignedPtr& trx) {
  if (!trx) return false;

  if (!trx->verify()) {
    return false;
  }

  if (_head) {
    if (trx->getExpire() <= _head->getHeight()) {
      return false;
    }
  }

  // 脚本是否可执行
  Script script = trx->getScript();
  const std::vector<SignaturePair> v = trx->getSignatures();
  if (scri::Transfer* t = boost::get<scri::Transfer>(&script)) {
    if (1 != v.size() || _account.query(v[0].signer) < t->getAmount()) {
      return false;
    }
  } else {
    int count = 0;
    for (auto i : v) {
      if (_bps.contains(i.signer)) {
        count++;
      }
    }
    uint32_t amount = _bps.getBPAmount();
    if (0 == amount ||
        (float)(count / amount) <= blockmirror::BP_PERCENT_SIGNER) {
      return false;
    }
  }

  if (_transaction.contains(trx->getHashPtr())) {
    return false;
  }

  return true;
}

bool Context::check(const chain::BlockHeaderSignedPtr& block) {
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