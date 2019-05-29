#include <blockmirror/chain/context.h>
#include <blockmirror/common.h>
#include <blockmirror/crypto/ecc.h>
#include <blockmirror/store/account_store.h>
#include <blockmirror/store/bps_store.h>
#include <blockmirror/store/data_store.h>
#include <blockmirror/store/format_store.h>
#include <blockmirror/store/transaction_store.h>
#include <boost/algorithm/hex.hpp>
#include <boost/test/unit_test.hpp>
#include "test_helper.h"

BOOST_AUTO_TEST_SUITE(context_tests2)

BOOST_AUTO_TEST_CASE(context_tests2_seq) {
  removeContextFiles();

  std::vector<blockmirror::chain::BlockPtr> blocks;
  std::vector<blockmirror::Privkey> privs;
  std::vector<blockmirror::Pubkey> pubs;

  // 1. 产生并执行10个区块
  privs.resize(10);
  pubs.resize(privs.size());
  for (size_t i = 0; i < privs.size(); i++) {
    blockmirror::crypto::ECC.newKey(privs[i]);
    blockmirror::crypto::ECC.computePub(privs[i], pubs[i]);
  }
  {
    blockmirror::chain::Context context;
    context.load();
    // 增加创世BP
    context.getBpsStore().add(pubs[0]);

    auto now = blockmirror::alignTimestamp(blockmirror::now_ms_since_1970());
    for (size_t i = 0; i < privs.size(); i++) {
      if (i != privs.size() - 1) {
        // 前面的BP联合将后面的BP加进去
        auto trx = std::make_shared<blockmirror::chain::TransactionSigned>(
            blockmirror::chain::scri::BPJoin(pubs[i + 1]));
        trx->setExpire(10);
        trx->setNonce();
        for (size_t j = 0; j <= i; j++) {
          trx->addSign(privs[j]);
        }
        context.getTransactionStore().add(trx);
      }

      auto blk = context.genBlock(privs[i], pubs[i], now);
      now += blockmirror::BLOCK_PER_MS;
      BOOST_CHECK_NE(blk, nullptr);
      BOOST_CHECK_EQUAL(blk->getHeight(), i + 1);
      /*
      std::cout << blk->getHeight() << " hash:";
      boost::algorithm::hex(blk->getHash(),
      std::ostream_iterator<char>(std::cout)); std::cout << " prev:";
      boost::algorithm::hex(blk->getPrevious(),
      std::ostream_iterator<char>(std::cout)); std::cout << std::endl;
      */
      blocks.push_back(blk);
    }
    context.close();
  }
  // 2. 查看余额是否正确
  {
    blockmirror::store::AccountStore account;
    account.load(".");
    for (auto &pub : pubs) {
      BOOST_CHECK_EQUAL(account.query(pub), blockmirror::MINER_AMOUNT);
    }
  }
  // 3. 查看区块是否存在
  {
    blockmirror::store::BlockStore store;
    store.load(".");
    for (auto &blk : blocks) {
      BOOST_CHECK(store.contains(blk->getHash()));
      BOOST_CHECK(store.getBlock(blk->getHash()));
    }
  }
  // 4. 回滚
  {
    blockmirror::chain::Context context;
    context.load();
    for (size_t i = 0; i < privs.size(); ++i) {
      BOOST_CHECK(context.rollback());
    }
    context.close();
  }
  // 5. 查看余额是否回滚
  {
    blockmirror::store::AccountStore account;
    account.load(".");
    for (auto &pub : pubs) {
      BOOST_CHECK_EQUAL(account.query(pub), 0);
    }
  }

  // 6.存mongo db
  /*blockmirror::globalConfig.init("config.json");
  for (auto pos = blocks.begin(); pos != blocks.end(); ++pos) {
	  blockmirror::store::MongoStore::get().save(*pos);
  }*/
}

BOOST_AUTO_TEST_SUITE_END()