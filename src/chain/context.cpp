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
        B_TRACE("unknown producer {:spn}", spdlog::to_hex(i.signer));
        return false;
      }
    }
    uint32_t amount = _context._bps.getBPAmount();
    if (amount == 0) return true;
    if (((float)sigs.size() / amount * 100) < blockmirror::BP_PERCENT_SIGNER) {
      B_TRACE("signatures low percent {}", (float)sigs.size() / amount * 100);
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
      B_TRACE("{:spn} amount {} < {}", spdlog::to_hex(signer.signer), amount,
              t.getAmount());
      return false;
    }
    return true;
  }
  bool operator()(const scri::BPJoin& b) const {
    if (!_checkBPSigner()) return false;
    if (_context._bps.contains(b.getBP())) {
      B_TRACE("{:spn} none exists", spdlog::to_hex(b.getBP()));
      return false;
    }
    return true;
  }
  bool operator()(const scri::BPExit& b) const {
    if (!_checkBPSigner()) return false;
    if (!_context._bps.contains(b.getBP())) {
      B_TRACE("{:spn} already exists", spdlog::to_hex(b.getBP()));
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
    _context._bpChanged = true;
    if (0 == _type) {
      return _context._bps.add(b.getBP());
    } else if (1 == _type) {
      return _context._bps.remove(b.getBP());
    }
    return false;
  }
  bool operator()(const scri::BPExit& b) const {
    _context._bpChanged = true;
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
  _dataSignature.load(path);

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
  _dataSignature.close();
}

bool Context::_apply(const TransactionSignedPtr& trx, bool rollback) {
  const std::vector<SignaturePair>& v = trx->getSignatures();
  if (v.empty()) {
    B_WARN("empty signatures {:spn}", spdlog::to_hex(trx->getHash()));
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

chain::BlockPtr Context::genBlock(const Privkey& key, const Pubkey& reward,
                                  uint64_t testTime) {
  chain::BlockPtr newBlock = std::make_shared<chain::Block>();
  if (!_head) {
    newBlock->setGenesis();
  } else {
    newBlock->setPrevious(*_head);
  }
  if (testTime != 0) {
    newBlock->setTimestamp(testTime);
  }

  _bpChanged = false;  // 清除状态
  Pubkey pub;
  crypto::ECC.computePub(key, pub);

  // 检查BP是否存在
  auto slot = _bps.find(pub);
  if (slot < 0) {
    B_WARN("producer none exists: {:spn}", spdlog::to_hex(pub));
    return nullptr;
  }
  // 检查时间戳和槽位是否正确
  auto slotByTime = 0;
  if (_head) {
    slotByTime = _bps.getSlotByTime(newBlock->getTimestamp());
    B_LOG("get slot by time {} {}", slotByTime, newBlock->getTimestamp());
  } else {
    B_LOG("get slot by genesis");
  }
  if (slotByTime != slot) {
    B_WARN("not producer turn: byTime({}) byPubkey({})", slotByTime, slot);
    return nullptr;
  }

  // 执行coinbase
  if (!_account.add(reward, MINER_AMOUNT)) {
    B_WARN("coinbase execute failed");
    return nullptr;
  }

  newBlock->setCoinbase(reward);
  _head = newBlock;

  auto trxs = _transaction.popUnpacked();
  B_LOG("packing unconfirm transaction count {}", trxs.size());
  // 执行所有交易
  for (auto& trx : trxs) {
    if (_apply(trx)) {
      newBlock->addTransaction(trx);
    } else {
      B_WARN("bad trx: {:spn}", spdlog::to_hex(trx->getHash()));
    }
  }

  std::vector<chain::DataSignedPtr> v = _dataSignature.pop();
  if (v.size() > 0) {
    DataBPPtr dataBP =
        std::make_shared<blockmirror::chain::DataBP>(globalConfig.miner_pubkey);
    for (auto i : v) {
      dataBP->addData(i);
    }
    newBlock->addDataBP(dataBP);
  }

  newBlock->finalize(key);
  _block.addBlock(newBlock);

  if (_bpChanged || _head->getHeight() == 1) {
    _bps.pushBpChange(slot, _head->getTimestamp());
  }

  B_LOG("block produced: {:spn}", spdlog::to_hex(newBlock->getHash()));

  return newBlock;
}

bool Context::apply(const chain::BlockPtr& block) {
  _bpChanged = false;

  // 检查区块是否和当前HEAD呈现链式连接
  if (_head) {
    if (block->getHeight() != _head->getHeight() + 1) {
      B_WARN("bad height head({}) block({})", _head->getHeight(),
             block->getHeight());
      return false;
    }
    if (block->getPrevious() != _head->getHash()) {
      B_WARN("must switch branch");
      return false;
    }
  } else {
    if (block->getHeight() != 1) {
      B_WARN("height must be 1");
      return false;
    }
    if (block->getPrevious() != Hash256{0}) {
      B_WARN("bad genesis previous");
      return false;
    }
  }
  // 检查BP是否存在
  auto slot = _bps.find(block->getProducer());
  if (slot < 0) {
    B_WARN("producer none exists: {:spn}",
           spdlog::to_hex(block->getProducer()));
    return false;
  }
  // 检查时间戳和槽位是否正确
  auto slotByTime = 0;
  if (_head) {
    slotByTime = _bps.getSlotByTime(block->getTimestamp());
    B_LOG("get slot by time {} {}", slotByTime, block->getTimestamp());
  } else {
    B_LOG("get slot by genesis");
  }
  if (slotByTime != slot) {
    B_WARN("producer bad slot: byTime({}) byPubkey({})", slotByTime, slot);
    return false;
  }
  // 检查coinbases是否正确
  const auto& coinbase = boost::get<blockmirror::chain::scri::Transfer>(
      block->getCoinbase()->getScript());
  if (coinbase.getAmount() != MINER_AMOUNT) {
    B_WARN("coinbase amount bad: {}", coinbase.getAmount());
    return false;
  }

  // 执行coinbase
  if (!_account.add(coinbase.getTarget(), coinbase.getAmount())) {
    B_WARN("coinbase execute failed");
    return false;
  }

  auto backup = _head;
  _head = block;

  // 执行所有交易
  const std::vector<TransactionSignedPtr> v = block->getTransactions();
  auto it = v.begin();
  for (; it != v.end(); ++it) {
    if (!_apply(*it)) {
      B_WARN("exec failed: {:spn} try revert",
             spdlog::to_hex((*it)->getHash()));
      break;
    }
  }

  // 如果没有执行完全则拒绝该区块必须回滚
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

  // 第一个区块 或者 BP发生变动 添加BP更改记录
  if (_bpChanged || _head->getHeight() == 1) {
    _bps.pushBpChange(slot, _head->getTimestamp());
  }

  return true;
}

bool Context::rollback() {
  _bpChanged = false;

  if (!_head) {
    B_WARN("no head");
    return false;
  }
  const auto& coinbase = boost::get<blockmirror::chain::scri::Transfer>(
      _head->getCoinbase()->getScript());
  // 回滚所有交易
  const std::vector<TransactionSignedPtr> v = _head->getTransactions();
  auto it = v.rbegin();
  for (; it != v.rend(); ++it) {
    if (!_apply(*it, true)) {
      B_WARN("rollback failed: {:spn} try revert",
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

  // 如果回滚时区块发生更改 或者 回滚创世块
  if (_bpChanged || !_head) {
    _bps.popBpChange();
  }

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
    B_LOG("nullptr block");
    return false;
  }
  if (!isAlignedTime(block->getTimestamp())) {
    B_WARN("bad align timestamp");
    return false;
  }

  if (_head) {
    // 最大可回滚
    if (block->getHeight() + BLOCK_MAX_ROLLBACK < _head->getHeight()) {
      B_LOG("too old block");
      return false;
    }
  }

  // FIXME: 这里有隐含的问题
  // 1. 当此区块的前面区块包含BP变动信息且前面区块尚未apply时
  // 2. 当发生分叉时
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

bool Context::check(const chain::DataPtr& data) {
  if (!data) {
    B_LOG("nullptr data");
    return false;
  }

  store::NewDataPtr newData = _data.query(data->getName());
  if (!newData) {
    return false;
  }

  store::NewFormatPtr newFormat = _format.query(newData->getFormat());
  if (!newFormat) {
    return false;
  }

  const std::vector<uint8_t> dataFormat = newFormat->getDataFormat();
  size_t dataSize = 0;
  for (const auto i : dataFormat) {
    switch (i) {
      case chain::scri::NewFormat::TYPE_FLOAT:
        dataSize += sizeof(float);
        break;
      case chain::scri::NewFormat::TYPE_DOUBLE:
        dataSize += sizeof(double);
        break;
      case chain::scri::NewFormat::TYPE_UINT:
        dataSize += sizeof(uint32_t);
        break;
      case chain::scri::NewFormat::TYPE_INT:
        dataSize += sizeof(int32_t);
        break;
      default:
        return false;
    }
  }

  if (dataSize != data->getData().size() * sizeof(uint8_t)) {
    return false;
  }

  return true;
}

}  // namespace chain
}  // namespace blockmirror