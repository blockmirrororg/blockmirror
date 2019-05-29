#pragma once

#include <blockmirror/chain/transaction.h>
#include <blockmirror/common.h>
#include <blockmirror/store/account_store.h>
#include <blockmirror/store/block_store.h>
#include <blockmirror/store/bps_store.h>
#include <blockmirror/store/data_signature_store.h>
#include <blockmirror/store/data_store.h>
#include <blockmirror/store/format_store.h>
#include <blockmirror/store/transaction_store.h>
//#include <blockmirror/store/mongo_store.h>

namespace blockmirror {
namespace chain {

class Context {
 private:
  friend class StoreVisitor;
  friend class CheckVisitor;
  store::AccountStore _account;
  store::BlockStore _block;
  store::BpsStore _bps;
  store::DataStore _data;
  store::FormatStore _format;
  store::TransactionStore _transaction;
  store::DataSignatureStore _dataSignature;
  //store::MongoStore _mongo;
  chain::BlockPtr _head;
 
  bool _loaded;
  bool _bpChanged;
  /**
   * @brief 执行一笔交易
   *
   * @param trx
   * @param rollback
   */
  bool _apply(const chain::TransactionSignedPtr& trx, bool rollback = false);

 public:
  Context();
  ~Context();

  void load();
  void close();
  // boost::asio::io_context& getMainContext() { return _mainContext; }
  // boost::asio::io_context& getWorkContext() { return _workContext; }

  chain::BlockPtr genBlock(const Privkey& key, const Pubkey& reward,
                           uint64_t testTime = 0);
  /**
   * @brief 执行一个区块
   *
   * @param block
   */
  bool apply(const chain::BlockPtr& block);

  /**
   * @brief 回滚一个区块
   */
  bool rollback();

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

  /**
   * @brief 检测数据是否可能被接受
   * @param data
   * @return true
   * @return false
   */
  bool check(const chain::DataPtr& data);

  template <typename T>
  bool hasBlock(const T& hash) {
    return _block.contains(hash);
  }

  store::TransactionStore& getTransactionStore() { return _transaction; }
  store::DataStore& getDataStore() { return _data; }
  store::DataSignatureStore& getDataSignatureStore() { return _dataSignature; }
  store::BpsStore& getBpsStore() { return _bps; }
  chain::BlockPtr& getHead() { return _head; }
  store::BlockStore& getBlockStore() { return _block; }
  store::FormatStore& getFormatStore() { return _format; }
  //store::MongoStore& getMongoStore() { return _mongo; }
};

}  // namespace chain
}  // namespace blockmirror