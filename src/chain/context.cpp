#include <blockmirror/chain/context.h>
#include <blockmirror/store/store.h>

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
        B_TRACE("unknown producer {}", spdlog::to_hex(i.signer));
        return false;
      }
    }
    uint32_t amount = _context._bps.getBPAmount();
    if (amount == 0) return true;
    if ((float)(sigs.size() / amount) < blockmirror::BP_PERCENT_SIGNER) {
      B_TRACE("signatures low percent {}", (float)(sigs.size() / amount));
      return false;
    }
    return true;
  }

 public:
  CheckVisitor(Context& context, const TransactionSignedPtr& trx)
      : _context(context), _transaction(trx){};
  bool operator()(const scri::Transfer& t) const {
    if (_transaction->getSignatures().size() != 1) {
      B_TRACE("signatures count 1 != {}", _transaction->getSignatures().size());
      return false;
    }
    auto& signer = *_transaction->getSignatures().begin();
    auto amount = _context._account.query(signer.signer);
    if (amount < t.getAmount()) {
      B_TRACE("{} amount {} < {}", spdlog::to_hex(signer.signer), amount,
              t.getAmount());
      return false;
    }
    return true;
  }
  bool operator()(const scri::BPJoin& b) const {
    if (!_checkBPSigner()) return false;
    if (_context._bps.contains(b.getBP())) {
      B_TRACE("{} none exists", spdlog::to_hex(b.getBP()));
      return false;
    }
    return true;
  }
  bool operator()(const scri::BPExit& b) const {
    if (!_checkBPSigner()) return false;
    if (!_context._bps.contains(b.getBP())) {
      B_TRACE("{} already exists", spdlog::to_hex(b.getBP()));
      return false;
    }
    return true;
  }
  bool operator()(const scri::NewFormat& n) const {
    if (!_checkBPSigner()) return false;
    if (_context._format.query(n.getName())) {
      B_TRACE("{} none exists", n.getName());
      return false;
    }
    return true;
  }
  bool operator()(const scri::NewData& n) const {
    if (!_checkBPSigner()) return false;
    if (_context._data.query(n.getName())) {
      B_TRACE("{} already exists", n.getName());
      return false;
    }
    return true;
  }
};

class StoreVisitor : public boost::static_visitor<bool> {
 private:
  Context& _context;
  const Pubkey& _signer;
  uint8_t _type;  // 0 正向操作 1 反向操作

 public:
  StoreVisitor(Context& context, const Pubkey& signer, uint8_t type)
      : _context(context), _signer(signer), _type(type){};
  bool operator()(const scri::Transfer& t) const {
    if (0 == _type) {
      return _context._account.transfer(_signer, t.getTarget(), t.getAmount());
    } else if (1 == _type) {
      return _context._account.transfer(t.getTarget(), _signer, t.getAmount());
    }
    return false;
  }
  bool operator()(const scri::BPJoin& b) const {
    if (0 == _type) {
      return _context._bps.add(b.getBP());
    } else if (1 == _type) {
      return _context._bps.remove(b.getBP());
    }
    return false;
  }
  bool operator()(const scri::BPExit& b) const {
    if (0 == _type) {
      return _context._bps.remove(b.getBP());
    } else if (1 == _type) {
      return _context._bps.add(b.getBP());
    }
    return false;
  }
  bool operator()(const scri::NewFormat& n) const {
    store::NewFormatPtr nPtr = std::make_shared<scri::NewFormat>(n);
    if (0 == _type) {
      return _context._format.add(nPtr);
    } else if (1 == _type) {
      return _context._format.remove(nPtr->getName());
    }
    return false;
  }
  bool operator()(const scri::NewData& n) const {
    store::NewDataPtr nPtr = std::make_shared<scri::NewData>(n);
    if (0 == _type) {
      return _context._data.add(nPtr);
    } else if (1 == _type) {
      return _context._data.remove(nPtr->getName());
    }
    return false;
  }
};

Context::Context() : _loaded(false) {}

Context::~Context() {
  if (_loaded) {
    close();
  }
}

void Context::load() {
  ASSERT(!_loaded);
  _loaded = true;
  boost::filesystem::path path = ".";
  _account.load(path);
  _block.load(path);
  _bps.load(path);
  _data.load(path);
  _format.load(path);
  _transaction.load(path);

  if (boost::filesystem::exists(path / "head")) {
    store::BinaryReader reader;
    reader.open(path / "head");
    reader >> _head;
  } else {
  }
}
void Context::close() {
  ASSERT(_loaded);
  _loaded = false;
  boost::filesystem::path path = ".";
  if (_head) {
    store::BinaryWritter writter;
    writter.open(path / "head");
    writter << _head;
  } else {
    boost::filesystem::remove(path / "head");
  }
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
    B_WARN("empty signatures {}", spdlog::to_hex(trx->getHash()));
    return false;
  }

  if (!boost::apply_visitor(StoreVisitor(*this, v[0].signer, rollback ? 1 : 0),
                            trx->getScript())) {
    return false;
  }

  // 添加或者修改交易的打包高度 回撤应该修改为0未打包状态
  _transaction.add(trx, rollback ? 0 : _head->getHeight());

  return true;
}

chain::BlockPtr Context::genBlock(const Privkey& key, const Pubkey& reward) {
  chain::BlockPtr newBlock = std::make_shared<chain::Block>();
  if (!_head) {
    newBlock->setGenesis();
  } else {
    newBlock->setPrevious(*_head);
  }
  // 执行coinbase
  if (!_account.add(reward, MINER_AMOUNT)) {
    B_WARN("coinbase execute failed");
    return nullptr;
  }
  newBlock->setCoinbase(reward);

  auto trxs = _transaction.popUnpacked();
  // 执行所有交易
  for (auto& trx : trxs) {
    if (_apply(trx)) {
      newBlock->addTransaction(trx);
    } else {
      B_WARN("bad trx: {}", spdlog::to_hex(trx->getHash()));
    }
  }
  // FIXME: 将临时数据池的所有数据添加到区块中

  newBlock->finalize(key);
  _head = newBlock;
  _block.addBlock(newBlock);
  return newBlock;
}

bool Context::apply(const chain::BlockPtr& block) {
  if (_head) {
    if (block->getHeight() != _head->getHeight() + 1) {
      B_WARN("bad height head({}) block({})", _head->getHeight(),
             block->getHeight());
      return false;
    }
  } else {
    if (block->getHeight() != 1) {
      B_WARN("height must be 1");
      return false;
    }
  }

  // coinbase
  const auto& coinbase = boost::get<blockmirror::chain::scri::Transfer>(
      block->getCoinbase()->getScript());
  if (coinbase.getAmount() != MINER_AMOUNT) {
    B_WARN("coinbase amount bad: {}", coinbase.getAmount());
    return false;
  }
  if (!_account.add(coinbase.getTarget(), coinbase.getAmount())) {
    B_WARN("coinbase execute failed");
    return false;
  }

  auto backup = _head;
  _head = block;

  const std::vector<TransactionSignedPtr> v = block->getTransactions();
  auto it = v.begin();
  for (; it != v.end(); ++it) {
    if (!_apply(*it)) {
      B_WARN("exec failed: {} try revert", spdlog::to_hex((*it)->getHash()));
      break;
    }
  }
  if (it != v.end()) {
    _head = backup;
    for (auto i = v.begin(); i != it; ++i) {
      if (!_apply(*i, true)) {
        throw std::runtime_error("bad apply");
      }
    }
    if (!_account.add(coinbase.getTarget(), -(int64_t)coinbase.getAmount())) {
      throw std::runtime_error("bad apply");
    }
    return false;
  }

  return true;
}

bool Context::rollback() {
  if (!_head) {
    B_WARN("no head");
    return false;
  }
  // coinbase
  const auto& coinbase = boost::get<blockmirror::chain::scri::Transfer>(
      _head->getCoinbase()->getScript());
  const std::vector<TransactionSignedPtr> v = _head->getTransactions();
  auto it = v.rbegin();
  for (; it != v.rend(); ++it) {
    if (!_apply(*it, true)) {
      B_WARN("rollback failed: {} try revert",
             spdlog::to_hex((*it)->getHash()));
      break;
    }
  }
  bool shouldRevert = (it != v.rend());
  if (!shouldRevert) {
    if (!_account.add(coinbase.getTarget(), -(int64_t)coinbase.getAmount())) {
      B_WARN("rollback coinbase failed try revert");
      shouldRevert = true;
    }
  }
  if (shouldRevert) {
    for (auto i = v.rbegin(); i != it; ++i) {
      if (!_apply(*i)) {
        throw std::runtime_error("bad rollback");
      }
    }
    return false;
  }

  _head = _block.getBlock(_head->getPrevious());
  return true;
}

bool Context::check(const chain::TransactionSignedPtr& trx) {
  if (!trx) {
    B_LOG("bad transaction");
    return false;
  };

  if (_transaction.contains(trx->getHashPtr())) {
    B_LOG("duplicate transaction");
    return false;
  }

  if (!trx->verify()) {
    B_LOG("bad transaction signatures");
    return false;
  }

  if (_head) {
    if (trx->getExpire() <= _head->getHeight()) {
      B_LOG("bad transaction expire");
      return false;
    }
  }

  if (!boost::apply_visitor(CheckVisitor(*this, trx), trx->getScript())) {
    return false;
  }

  return true;
}

bool Context::check(const chain::BlockHeaderSignedPtr& block) {
  if (!block) {
    B_LOG("bad block");
    return false;
  }

  if (_head) {
    // 最大可回滚
    if (block->getHeight() + BLOCK_MAX_ROLLBACK < _head->getHeight()) {
      B_LOG("too old block");
      return false;
    }
  }

  // FIXME: 这里有隐含的问题，具体情况以后考虑
  if (!_bps.contains(block->getProducer())) {
    B_LOG("bad producer");
    return false;
  }

  if (!block->verify()) {
    B_LOG("bad block signatures");
    return false;
  }

  return true;
}

}  // namespace chain
}  // namespace blockmirror