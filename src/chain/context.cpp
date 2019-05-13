#include <blockmirror/chain/context.h>

namespace blockmirror {
namespace chain {

class CheckVisitor : public boost::static_visitor<bool> {
 private:
  Context& _context;
  const TransactionSignedPtr& _transaction;

  bool _checkBPSigner() const {
    auto& sigs = _transaction->getSignatures();
    for (auto& i : sigs) {
      if (!_context._bps.contains(i.signer)) {
        return false;
      }
    }
    uint32_t amount = _context._bps.getBPAmount();
    if (amount == 0) return true;
    if ((float)(sigs.size() / amount) < blockmirror::BP_PERCENT_SIGNER) {
      return false;
    }
    return true;
  }

 public:
  CheckVisitor(Context& context, const TransactionSignedPtr& trx)
      : _context(context), _transaction(trx){};
  bool operator()(const scri::Transfer& t) const {
    if (_transaction->getSignatures().size() != 1) return false;
    auto& signer = *_transaction->getSignatures().begin();
    if (_context._account.query(signer.signer) < t.getAmount()) return false;
    return true;
  }
  bool operator()(const scri::BPJoin& b) const {
    if (!_checkBPSigner()) return false;
    if (_context._bps.contains(b.getBP())) return false;
    return true;
  }
  bool operator()(const scri::BPExit& b) const {
    if (!_checkBPSigner()) return false;
    if (!_context._bps.contains(b.getBP())) return false;
    return true;
  }
  bool operator()(const scri::NewFormat& n) const {
    if (!_checkBPSigner()) return false;
    if (_context._format.query(n.getName())) return false;
    return true;
  }
  bool operator()(const scri::NewData& n) const {
    if (!_checkBPSigner()) return false;
    if (_context._data.query(n.getName())) return false;
    return true;
  }
};

class StoreVisitor : public boost::static_visitor<bool> {
 private:
  Context& _context;
  Pubkey& _signer;
  uint8_t _type;  // 0 正向操作 1 反向操作

 public:
  StoreVisitor(Context& context, Pubkey& signer, uint8_t type)
      : _context(context), _signer(signer), _type(type){};
  bool operator()(const scri::Transfer& t) const {
    if (0 == _type) {
      return _context._account.transfer(_signer, t.getTarget(), t.getAmount());
    } else if (1 == _type) {
      return _context._account.transfer(t.getTarget(), _signer, t.getAmount());
    }
  }
  bool operator()(const scri::BPJoin& b) const {
    if (0 == _type) {
      return _context._bps.add(b.getBP());
    } else if (1 == _type) {
      return _context._bps.remove(b.getBP());
    }
  }
  bool operator()(const scri::BPExit& b) const {
    if (0 == _type) {
      return _context._bps.remove(b.getBP());
    } else if (1 == _type) {
      return _context._bps.add(b.getBP());
    }
  }
  bool operator()(const scri::NewFormat& n) const {
    store::NewFormatPtr nPtr = std::make_shared<scri::NewFormat>(n);
    if (0 == _type) {
      return _context._format.add(nPtr);
    } else if (1 == _type) {
      return _context._format.remove(nPtr->getName());
    }
  }
  bool operator()(const scri::NewData& n) const {
    store::NewDataPtr nPtr = std::make_shared<scri::NewData>(n);
    if (0 == _type) {
      return _context._data.add(nPtr);
    } else if (1 == _type) {
      return _context._data.remove(nPtr->getName());
    }
  }
};

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

bool Context::_apply(const TransactionSignedPtr& trx, bool rollback) {
  const std::vector<SignaturePair>& v = trx->getSignatures();
  if (v.empty()) {
    return false;
  }

  Pubkey signer = v[0].signer;
  Script script = trx->getScript();
  if (!boost::apply_visitor(StoreVisitor(*this, signer, rollback ? 1 : 0),
                            script)) {
    return false;
  }

  bool ret = _transaction.add(trx, rollback ? 0 : _head->getHeight());
  if (!rollback && !ret) {
    return false;
  } else if (rollback && ret) {
    return false;
  }

  return true;
}

bool Context::apply(const chain::BlockPtr& block) {
  if (_head) {
    if (block->getHeight() != _head->getHeight() + 1) {
      return false;
    }
  } else {
    if (block->getHeight() != 1) {
      return false;
    }
  }

  auto backup = _head;
  _head = block;

  const std::vector<TransactionSignedPtr> v = block->getTransactions();
  auto it = v.begin();
  for (; it != v.end(); ++it) {
    if (!_apply(*it)) {
      break;
    }
  }
  if (it != v.end() || !_account.add(block->getProducer(), MINER_AMOUNT)) {
    _head = backup;
    for (auto i = v.begin(); i != it; ++i) {
      if (!_apply(*i, true)) {
        throw std::runtime_error("bad apply");
      }
    }
    return false;
  }

  return true;
}

bool Context::rollback() {
  if (!_head) {
    return false;
  }
  const std::vector<TransactionSignedPtr> v = _head->getTransactions();
  auto it = v.rbegin();
  for (; it != v.rend(); ++it) {
    if (!_apply(*it, true)) {
      break;
    }
  }
  if (it != v.rend()) {
    for (auto i = v.rbegin(); i != it; ++i) {
      if (!_apply(*i)) {
        throw std::runtime_error("bad rollback");
      }
    }
  }

  _head = _block.getBlock(_head->getPrevious());
  return true;
}

bool Context::check(const chain::TransactionSignedPtr& trx) {
  if (!trx) return false;

  if (_transaction.contains(trx->getHashPtr())) {
    return false;
  }

  if (!trx->verify()) {
    return false;
  }

  if (_head) {
    if (trx->getExpire() <= _head->getHeight()) {
      return false;
    }
  }

  // by lvjl
  // 下面2行在vs2017编译不过
  /*if (!boost::apply_visitor(CheckVisitor(*this, trx), trx->getScript())) {
    return false;
  }*/

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