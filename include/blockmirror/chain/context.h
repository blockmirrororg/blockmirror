#pragma once

#include <blockmirror/chain/transaction.h>
#include <blockmirror/common.h>
#include <blockmirror/store/account_store.h>
#include <blockmirror/store/block_store.h>
#include <blockmirror/store/bps_store.h>
#include <blockmirror/store/data_store.h>
#include <blockmirror/store/format_store.h>
#include <blockmirror/store/transaction_store.h>

namespace blockmirror {
namespace chain {

class Context {
 private:
  friend class StoreVisitor;
  store::AccountStore _account;
  store::BlockStore _block;
  store::BpsStore _bps;
  store::DataStore _data;
  store::FormatStore _format;
  store::TransactionStore _transaction;

  chain::BlockPtr _head;

  /**
   * @brief 执行一笔交易
   *
   * @param trx
   */
  void _apply(const chain::TransactionSignedPtr& trx);
  /**
   * @brief 回滚一笔交易
   *
   * @param trx
   */
  void _rollback(const chain::TransactionSignedPtr& trx);

 public:
  Context();
  ~Context();

  void load();
  void close();

  /**
   * @brief 执行一个区块
   *
   * @param block
   */
  void apply(const chain::BlockPtr& block);

  /**
   * @brief 回滚一个区块
   *
   * @param block
   */
  void rollback(const chain::BlockPtr& block);

  /**
   * @brief 检测当前交易是否可能被接受
   * 1. 签名 以及 是否过期
   * 2. 脚本是否可执行
   * 3. 是否已经存在
   * @param trx
   * @return true
   * @return false
   */
  bool check(const chain::TransactionSignedPtr& trx);

  /**
   * @brief 检测当前区块是否可能被接受
   * 1. 签名 以及 高度是否可接受
   * @param block
   * @return true
   * @return false
   */
  bool check(const chain::BlockHeaderSignedPtr& block);

  template <typename T>
  bool hasBlock(const T& hash) {
    return _block.contains(hash);
  }

  static Context &get() {
    static Context context;
	return context;
  }

  store::TransactionStore& get_transaction_store() { return _transaction; }

  store::DataStore& get_data_store() { return _data; }
};

class StoreVisitor : public boost::static_visitor<> {
 private:
  Context& _context;
  Pubkey& _signer;
  uint8_t _type;  // 0 正向操作 1 反向操作

 public:
  StoreVisitor(Context& context, Pubkey& signer, uint8_t type)
      : _context(context), _signer(signer), _type(type){};
  void operator()(scri::Transfer& t) const {
    if (0 == _type) {
      _context._account.transfer(_signer, t.getTarget(), t.getAmount());
    } else if (1 == _type) {
      _context._account.transfer(t.getTarget(), _signer, t.getAmount());
    }
  }
  void operator()(scri::BPJoin& b) const {
    if (0 == _type) {
      _context._bps.add(b.getBP());
    } else if (1 == _type) {
      _context._bps.remove(b.getBP());
    }
  }
  void operator()(scri::BPExit& b) const {
    if (0 == _type) {
      _context._bps.remove(b.getBP());
    } else if (1 == _type) {
      _context._bps.add(b.getBP());
    }
  }
  void operator()(scri::NewFormat& n) const {
    store::NewFormatPtr nPtr = std::make_shared<scri::NewFormat>(n);
    if (0 == _type) {
      _context._format.add(nPtr);
    } else if (1 == _type) {
      _context._format.remove(nPtr->getName());
    }
  }
  void operator()(scri::NewData& n) const {
    store::NewDataPtr nPtr = std::make_shared<scri::NewData>(n);
    if (0 == _type) {
      _context._data.add(nPtr);
    } else if (1 == _type) {
      _context._data.remove(nPtr->getName());
    }
  }
};

}  // namespace chain
}  // namespace blockmirror