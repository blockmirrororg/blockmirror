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

BOOST_AUTO_TEST_SUITE(context_tests2)

BOOST_AUTO_TEST_CASE(context_tests2_seq) {
  boost::filesystem::remove("./0.block");
  boost::filesystem::remove("./index");
  boost::filesystem::remove("./account");
  boost::filesystem::remove("./bps");
  boost::filesystem::remove("./data");
  boost::filesystem::remove("./format");
  boost::filesystem::remove("./transaction");
  boost::filesystem::remove("./head");

  std::vector<blockmirror::chain::BlockPtr> blocks;
  std::vector<blockmirror::Privkey> privs;
  std::vector<blockmirror::Pubkey> pubs;

  // 1. 产生并执行10个区块
  privs.resize(10);
  pubs.resize(privs.size());
  {
    blockmirror::chain::Context context;
    context.load();
    for (size_t i = 0; i < privs.size(); i++) {
      blockmirror::crypto::ECC.newKey(privs[i]);
      blockmirror::crypto::ECC.computePub(privs[i], pubs[i]);

      auto blk = context.genBlock(privs[i], pubs[i]);
      BOOST_CHECK_NE(blk, nullptr);
      BOOST_CHECK_EQUAL(blk->getHeight(), i + 1);
      std::cout << blk->getHeight() << " hash:";
      boost::algorithm::hex(blk->getHash(), std::ostream_iterator<char>(std::cout));
      std::cout << " prev:";
      boost::algorithm::hex(blk->getPrevious(), std::ostream_iterator<char>(std::cout));
      std::cout << std::endl;
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
}

BOOST_AUTO_TEST_SUITE_END()