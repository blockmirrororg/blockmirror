#include <blockmirror/chain/context.h>
#include <blockmirror/common.h>
#include <blockmirror/store/account_store.h>
#include <blockmirror/store/bps_store.h>
#include <blockmirror/store/data_signature_store.h>
#include <blockmirror/store/data_store.h>
#include <blockmirror/store/format_store.h>
#include <blockmirror/store/transaction_store.h>
#include <boost/algorithm/hex.hpp>
#include <boost/test/unit_test.hpp>
#include "test_helper.h"

BOOST_AUTO_TEST_SUITE(context_tests)

//生成公私钥
std::vector<blockmirror::Privkey> pris;
std::vector<blockmirror::Pubkey> pubs;
static void createKey() {
  pris.resize(10);
  pubs.resize(10);
  for (size_t i = 0; i < pris.size(); i++) {
    blockmirror::crypto::ECC.newKey(pris[i]);
    blockmirror::crypto::ECC.computePub(pris[i], pubs[i]);
  }
}

//生成区块
blockmirror::chain::BlockPtr block[10];
static void createBlock() {
  for (auto i = 0; i < 10; i++) {
    block[i] = std::make_shared<blockmirror::chain::Block>();
  }
}

//生成各种交易
std::vector<blockmirror::chain::TransactionSignedPtr> tPtr;
static void inittPtr() {
  tPtr = {
      std::make_shared<blockmirror::chain::TransactionSigned>(
          blockmirror::chain::scri::Transfer(PUB(pubSunqi), 100)),  // 0

      std::make_shared<blockmirror::chain::TransactionSigned>(
          blockmirror::chain::scri::BPJoin({1, 2, 3})),  // 1

      std::make_shared<blockmirror::chain::TransactionSigned>(
          blockmirror::chain::scri::BPJoin({4, 5, 6})),  // 2

      std::make_shared<blockmirror::chain::TransactionSigned>(
          blockmirror::chain::scri::BPExit({4, 5, 6})),  // 3

      std::make_shared<blockmirror::chain::TransactionSigned>(
          blockmirror::chain::scri::NewData("aaa", "111", "rrt")),  // 4

      std::make_shared<blockmirror::chain::TransactionSigned>(
          blockmirror::chain::scri::NewFormat("aaa", "ggg", {1, 3, 5, 7, 9},
                                              {2, 4, 6, 8}, {6, 6, 6})),  // 5

      std::make_shared<blockmirror::chain::TransactionSigned>(
          blockmirror::chain::scri::BPJoin({4, 5, 6})),  // 6

      std::make_shared<blockmirror::chain::TransactionSigned>(
          blockmirror::chain::scri::NewFormat("bbb", "ccc", {15, 3, 55, 75, 9},
                                              {28, 4, 65, 8},
                                              {60, 76, 6})),  // 7

      std::make_shared<blockmirror::chain::TransactionSigned>(
          blockmirror::chain::scri::NewFormat(
              "aaa2", "fff", {12, 13, 15, 27, 9}, {2, 45, 65, 85},
              {96, 61, 126})),  // 8
      std::make_shared<blockmirror::chain::TransactionSigned>(
          blockmirror::chain::scri::BPJoin(PUB(pubZhaoliu))),  // 9

      std::make_shared<blockmirror::chain::TransactionSigned>(
          blockmirror::chain::scri::BPExit(PUB(pubZhaoliu))),  // 10

      std::make_shared<blockmirror::chain::TransactionSigned>(
          blockmirror::chain::scri::BPJoin(pubs[8])),  // 11

      std::make_shared<blockmirror::chain::TransactionSigned>(
          blockmirror::chain::scri::BPExit(pubs[8]))  // 12
  };
}

static void addBP() {
  blockmirror::store::BpsStore store;
  store.load(".");
  BOOST_CHECK(store.add(PUB(pubZhaoliu)));
  BOOST_CHECK(store.add(PUB(pubSunqi)));
}

static void saveBlock(int b, int e) {
  //将执行的区块保存下来（因为后面的回滚要查询区块）
  blockmirror::store::BlockStore store;
  store.load(".");

  for (size_t i = b; i <= e; i++) {
    BOOST_CHECK(store.addBlock(block[i]));

    BOOST_CHECK(store.contains(block[i]->getHashPtr()));
    BOOST_CHECK(store.contains(block[i]->getHash()));
  }
}

BOOST_AUTO_TEST_CASE(context_tests_ok1) {
  removeContextFiles();
  createKey();
  createBlock();
  inittPtr();
  addBP();

  //设置区块1
  block[1]->setGenesis();
  block[1]->setCoinbase(PUB(pubZhaoliu));
  block[1]->finalize(PRIV(privZhaoliu));

  //设置区块2
  block[2]->setPrevious(*block[1]);
  block[2]->setCoinbase(PUB(pubZhaoliu));
  // 两个BP 下一个时间时轮到 B 出
  // 所以要区块2必须 是下下一个时间 才轮到A
  block[2]->setTimestamp(block[1]->getTimestamp() +
                         2 * blockmirror::BLOCK_PER_MS);
  block[2]->finalize(PRIV(privZhaoliu));

  //设置区块2的交易
  for (size_t i = 0; i < 6; i++) {
    if (i == 0) {
      tPtr[i]->addSign(PRIV(privZhaoliu));  //这里是A转账给B
    } else {
      tPtr[i]->addSign(pris[i]);
    }

    block[2]->addTransaction(tPtr[i]);

    // B_LOG("{}", spdlog::to_hex(tPtr[i]->getHash()));
  }

  // 设置区块3
  block[3]->setPrevious(*block[2]);
  block[3]->setCoinbase(PUB(pubSunqi));
  block[3]->setTimestamp(block[2]->getTimestamp() + blockmirror::BLOCK_PER_MS);
  block[3]->finalize(PRIV(privSunqi));

  //设置区块3的交易
  for (size_t i = 6; i < 9; i++) {
    tPtr[i]->addSign(pris[i]);
    block[3]->addTransaction(tPtr[i]);
  }

  {  //执行区块1，2，3
    blockmirror::chain::Context c;
    c.load();
    BOOST_CHECK(c.apply(block[1]));
    BOOST_CHECK(c.apply(block[2]));
    BOOST_CHECK(c.apply(block[3]));
    c.close();
    //查询执行区块后存储的信息
    blockmirror::store::AccountStore accountStore;
    accountStore.load(".");
    BOOST_CHECK_EQUAL(accountStore.query(PUB(pubZhaoliu)), 199999900);
    BOOST_CHECK_EQUAL(accountStore.query(PUB(pubSunqi)), 100000100);

    blockmirror::store::BpsStore bpsStore;
    bpsStore.load(".");
    BOOST_CHECK_EQUAL(bpsStore.contains(PUB(pubZhaoliu)), 1);
    BOOST_CHECK_EQUAL(bpsStore.contains(PUB(pubSunqi)), 1);

    blockmirror::store::DataStore dataStore;
    dataStore.load(".");
    BOOST_CHECK(dataStore.query("111"));

    blockmirror::store::FormatStore formatStore;
    formatStore.load(".");
    BOOST_CHECK(formatStore.query("aaa"));
    BOOST_CHECK(formatStore.query("bbb"));

    blockmirror::store::TransactionStore transactionStore;
    transactionStore.load(".");
    for (size_t i = 0; i < 9; i++) {
      BOOST_CHECK_EQUAL(transactionStore.contains(tPtr[i]->getHashPtr()), 1);
    }
  }
  //保存区块1，2，3
  saveBlock(1, 3);

  {  //回滚区块
    blockmirror::chain::Context c;
    c.load();
    BOOST_CHECK(c.rollback());
    BOOST_CHECK(c.rollback());
    BOOST_CHECK(c.rollback());
    c.close();

    //查询回滚后的存储的信息
    blockmirror::store::AccountStore store;
    store.load(".");
    BOOST_CHECK_EQUAL(store.query(PUB(pubZhaoliu)), 0);
    BOOST_CHECK_EQUAL(store.query(PUB(pubSunqi)), 0);

    blockmirror::store::BpsStore bpsStore;
    bpsStore.load(".");
    BOOST_CHECK_EQUAL(bpsStore.contains({1, 2, 3}), 0);
    BOOST_CHECK_EQUAL(bpsStore.contains({4, 5, 6}), 0);

    blockmirror::store::DataStore dataStore;
    dataStore.load(".");
    BOOST_CHECK(!dataStore.query("111"));

    blockmirror::store::FormatStore formatStore;
    formatStore.load(".");
    BOOST_CHECK(!formatStore.query("aaa"));
    BOOST_CHECK(!formatStore.query("bbb"));

    blockmirror::store::TransactionStore transactionStore;
    transactionStore.load(".");
    for (size_t i = 0; i < 9; i++) {
      BOOST_CHECK_EQUAL(transactionStore.contains(tPtr[i]->getHashPtr()), 1);
    }
  }
}

BOOST_AUTO_TEST_CASE(context_tests_ok2) {
  removeContextFiles();
  createKey();
  createBlock();
  inittPtr();
  addBP();

  //设置区块4
  block[4]->setGenesis();
  block[4]->setCoinbase(PUB(pubZhaoliu));
  block[4]->finalize(PRIV(privZhaoliu));

  //设置区块4的交易
  tPtr[7]->addSign(PRIV(privSunqi));
  block[4]->addTransaction(tPtr[7]);

  //设置区块5
  block[5]->setPrevious(*block[4]);
  block[5]->setCoinbase(PUB(pubSunqi));
  block[5]->setTimestamp(block[4]->getTimestamp() + blockmirror::BLOCK_PER_MS);
  block[5]->finalize(PRIV(privSunqi));

  //设置区块5的交易
  tPtr[4]->addSign(PRIV(privSunqi));
  block[5]->addTransaction(tPtr[4]);

  tPtr[4]->addSign(PRIV(
      privZhaoliu));  //设置相同的NewData，导致这个区块执行交易回退，同时head回退
  block[5]->addTransaction(tPtr[4]);

  //设置区块6
  block[6]->setPrevious(*block[5]);
  block[6]->setCoinbase(PUB(pubSunqi));
  block[6]->setTimestamp(block[5]->getTimestamp() +
                         2 * blockmirror::BLOCK_PER_MS);
  block[6]->finalize(PRIV(privSunqi));

  //设置区块6的交易
  tPtr[5]->addSign(pris[1]);
  block[6]->addTransaction(
      tPtr[5]);  //由于区块5执行失败，由于height的关系，区块6也失败

  {  //执行区块4 5 6
    blockmirror::chain::Context c;
    c.load();
    BOOST_CHECK(c.apply(block[4]));
    BOOST_CHECK(!c.apply(block[5]));
    BOOST_CHECK(!c.apply(block[6]));
    c.close();
  }
}

BOOST_AUTO_TEST_CASE(context_tests_ok3) {
  removeContextFiles();
  createKey();
  createBlock();
  inittPtr();
  addBP();

  //设置区块4
  block[4]->setGenesis();
  block[4]->setCoinbase(PUB(pubZhaoliu));
  block[4]->finalize(PRIV(privZhaoliu));

  //设置区块4的交易
  tPtr[7]->addSign(pris[1]);
  block[4]->addTransaction(tPtr[7]);

  tPtr[0]->addSign(pris[8]);  // pris[8]给B转账100，但是pris[8]余额不足
  block[4]->addTransaction(tPtr[0]);

  {  //执行区块4
    blockmirror::chain::Context c;
    c.load();
    BOOST_CHECK(!c.apply(block[4]));
    c.close();
  }

  blockmirror::store::AccountStore store;
  store.load(".");
  // pris[8]给B转账100，但是失败，B余额是0。coinbase也进行了回滚，所以A余额是0
  BOOST_CHECK_EQUAL(store.query(PUB(pubZhaoliu)), 0);
  BOOST_CHECK_EQUAL(store.query(PUB(pubSunqi)), 0);

  blockmirror::store::TransactionStore transactionStore;
  transactionStore.load(".");
  BOOST_CHECK_EQUAL(transactionStore.contains(tPtr[7]->getHashPtr()), 1);
  // tPtr[7]的NewFormat因为tPtr[0]失败，tPtr[7]进行了回滚，并没有保存
  blockmirror::store::FormatStore formatStore;
  formatStore.load(".");
  BOOST_CHECK_EQUAL(formatStore.query("bbb"), nullptr);
}

BOOST_AUTO_TEST_CASE(context_tests_ok4) {
  removeContextFiles();
  createKey();
  createBlock();
  inittPtr();
  addBP();

  {  //还没执行过区块，回滚失败
    blockmirror::chain::Context c;
    c.load();
    BOOST_CHECK(!c.rollback());
    c.close();
  }

  //设置区块4
  block[4]->setGenesis();
  block[4]->setCoinbase(PUB(pubZhaoliu));
  block[4]->finalize(PRIV(privZhaoliu));

  //设置区块4的交易
  tPtr[7]->addSign(pris[1]);
  block[4]->addTransaction(tPtr[7]);

  tPtr[2]->addSign(pris[3]);
  block[4]->addTransaction(tPtr[2]);

  {  //执行区块4
    blockmirror::chain::Context c;
    c.load();
    BOOST_CHECK(c.apply(block[4]));
    c.close();
  }

  {  //为了让区块回滚失败，故意删掉tPtr[7]执行的交易数据
    blockmirror::store::FormatStore formatStore;
    formatStore.load(".");
    BOOST_CHECK(formatStore.remove("bbb"));
  }

  {  //回滚失败
    blockmirror::chain::Context c;
    c.load();
    BOOST_CHECK(!c.rollback());
    c.close();
  }

  blockmirror::store::AccountStore store;
  store.load(".");
  // 回滚失败，所以A余额是coinbase
  BOOST_CHECK_EQUAL(store.query(PUB(pubZhaoliu)), 100000000);

  blockmirror::store::BpsStore bpsStore;
  bpsStore.load(".");
  BOOST_CHECK_EQUAL(bpsStore.contains(PUB(pubSunqi)),
                    1);  //回滚失败，B仍然还是BP

  blockmirror::store::TransactionStore transactionStore;
  transactionStore.load(".");
  BOOST_CHECK_EQUAL(transactionStore.contains(tPtr[7]->getHashPtr()), 1);
}

BOOST_AUTO_TEST_CASE(context_tests_ok5) {
  removeContextFiles();
  createKey();
  createBlock();
  inittPtr();
  addBP();

  //设置区块5
  block[5]->setGenesis();
  block[5]->setCoinbase(PUB(pubZhaoliu));
  block[5]->finalize(PRIV(privZhaoliu));

  //设置区块5的交易,但是不签名
  block[5]->addTransaction(tPtr[3]);

  {  //执行区块5
    blockmirror::chain::Context c;
    c.load();
    BOOST_CHECK(!c.apply(block[5]));
    c.close();
  }
}

BOOST_AUTO_TEST_CASE(context_tests_ok6) {
  removeContextFiles();
  createKey();
  createBlock();
  inittPtr();
  addBP();

  //设置区块5
  block[5]->setGenesis();
  block[5]->setCoinbase(PUB(pubZhaoliu), 100);  //不等于MINER_AMOUNT
  block[5]->finalize(PRIV(privZhaoliu));

  //设置区块6
  block[6]->setPrevious(*block[5]);
  block[6]->setCoinbase(PUB(pubSunqi));
  block[3]->setTimestamp(block[5]->getTimestamp() + blockmirror::BLOCK_PER_MS);
  block[6]->finalize(PRIV(privSunqi));

  {  //先执行区块6,再执行区块5
    blockmirror::chain::Context c;
    c.load();
    BOOST_CHECK(!c.apply(block[6]));
    BOOST_CHECK(!c.apply(block[5]));
    c.close();
  }
}

BOOST_AUTO_TEST_CASE(context_tests_ok7) {
  removeContextFiles();
  createKey();
  createBlock();
  inittPtr();
  addBP();

  //设置区块5
  block[5]->setGenesis();
  block[5]->setCoinbase(PUB(pubZhaoliu));
  block[5]->finalize(PRIV(privZhaoliu));

  //设置区块5的交易
  tPtr[5]->addSign(pris[1]);
  block[5]->addTransaction(tPtr[5]);

  {  //先执行一遍这个交易，让下面的交易检查是否存在相同的交易
    blockmirror::chain::Context c;
    c.load();
    BOOST_CHECK(c.apply(block[5]));
    c.close();
  }

  {
    blockmirror::chain::Context c;
    c.load();

    BOOST_CHECK(!c.check(tPtr[5]));

    tPtr[6]->setExpire(0);
    tPtr[6]->addSign(PRIV(privZhaoliu));
    BOOST_CHECK(!c.check(tPtr[6]));

    tPtr[7]->setExpire(2);
    tPtr[7]->addSign(PRIV(privZhaoliu));
    tPtr[7]->addSign(PRIV(privSunqi));
    BOOST_CHECK(c.check(tPtr[7]));

    tPtr[8]->setExpire(2);
    tPtr[8]->addSign(PRIV(privZhaoliu));
    BOOST_CHECK(!c.check(tPtr[8]));

    blockmirror::chain::TransactionSignedPtr t;
    BOOST_CHECK(!c.check(t));

    //交易检查 没签名
    BOOST_CHECK(!c.check(tPtr[0]));
    BOOST_CHECK(!c.check(tPtr[5]));

    c.close();
  }
}

BOOST_AUTO_TEST_CASE(context_tests_ok8) {
  removeContextFiles();
  createKey();
  createBlock();
  inittPtr();
  addBP();

  //设置区块1
  block[1]->setGenesis();
  block[1]->setCoinbase(PUB(pubZhaoliu));
  block[1]->finalize(PRIV(privZhaoliu));

  //设置区块2
  block[2]->setPrevious(*block[1]);
  block[2]->setCoinbase(PUB(pubSunqi));
  block[2]->setTimestamp(block[1]->getTimestamp() + blockmirror::BLOCK_PER_MS);
  block[2]->finalize(PRIV(privSunqi));

  //设置区块3
  block[3]->setPrevious(*block[2]);
  block[3]->setCoinbase(PUB(pubSunqi));
  block[3]->setTimestamp(block[2]->getTimestamp() + blockmirror::BLOCK_PER_MS);
  block[3]->finalize(PRIV(privSunqi));

  //设置区块4
  block[4]->setPrevious(*block[2]);
  block[4]->setCoinbase(PUB(pubSunqi));
  block[4]->setTimestamp(block[2]->getTimestamp() +
                         2 * blockmirror::BLOCK_PER_MS);
  block[4]->finalize(PRIV(privSunqi));

  //设置区块4的交易
  tPtr[11]->addSign(PRIV(privZhaoliu));  //把 pubs[8] 加入BP
  tPtr[11]->addSign(PRIV(privSunqi));
  block[4]->addTransaction(tPtr[11]);

  {  //执行区块
    blockmirror::chain::Context c;
    c.load();
    BOOST_CHECK(c.apply(block[1]));
    BOOST_CHECK(c.apply(block[2]));
    BOOST_CHECK(!c.apply(block[3]));
    BOOST_CHECK(c.apply(block[4]));
    c.close();
  }

  //设置区块5
  block[5]->setPrevious(*block[4]);
  block[5]->setCoinbase(PUB(pubZhaoliu));
  block[5]->setTimestamp(
      block[4]->getTimestamp() +
      2 * blockmirror::BLOCK_PER_MS);  //因为pubs[8] 加入了BP，需要跳过pubs[8]
  block[5]->finalize(PRIV(privZhaoliu));

  //设置区块5的交易
  tPtr[10]->addSign(PRIV(privZhaoliu));  //删除BP A
  tPtr[10]->addSign(PRIV(privSunqi));
  block[5]->addTransaction(tPtr[10]);

  //设置区块6
  block[6]->setPrevious(*block[5]);
  block[6]->setCoinbase(PUB(pubSunqi));
  block[6]->setTimestamp(block[5]->getTimestamp() + blockmirror::BLOCK_PER_MS);
  block[6]->finalize(PRIV(privSunqi));

  //设置区块7
  block[7]->setPrevious(*block[5]);
  block[7]->setCoinbase(pubs[8]);
  block[7]->setTimestamp(block[5]->getTimestamp() + blockmirror::BLOCK_PER_MS);
  block[7]->finalize(pris[8]);

  {  //执行区块
    blockmirror::chain::Context c;
    c.load();
    BOOST_CHECK(c.apply(block[5]));
    /*这里是按位置来，所以区块6没过，区块7过，也就是：本来A位置是0，删除了之后，B位置变为0，
    pubs[8]位置变为1，执行A（0位置）之后的区块，就到1位置的pubs[8]*/
    BOOST_CHECK(!c.apply(block[6]));
    BOOST_CHECK(c.apply(block[7]));
    c.close();
  }
}

BOOST_AUTO_TEST_CASE(context_tests_ok9) {
  removeContextFiles();
  createKey();
  createBlock();
  inittPtr();
  addBP();

  {
    blockmirror::store::NewDataPtr pdata =
        std::make_shared<blockmirror::chain::scri::NewData>(
            std::string{"abc"}, std::string{"111"}, std::string{"+++"});

    blockmirror::store::DataStore dataStore;
    dataStore.load(".");
    dataStore.add(pdata);
    BOOST_CHECK(dataStore.query("111"));

    std::vector<uint8_t> v1{3, 3, 3};
    blockmirror::store::NewFormatPtr pformat =
        std::make_shared<blockmirror::chain::scri::NewFormat>(
            std::string{"111"}, std::string{"222"}, v1, v1, v1);

    blockmirror::store::FormatStore formatStore;
    formatStore.load(".");
    formatStore.add(pformat);
    BOOST_CHECK(formatStore.query("111"));
  }

  blockmirror::chain::DataPtr ptr1 = std::make_shared<blockmirror::chain::Data>(
      blockmirror::chain::Data{"111", {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}});

  blockmirror::chain::DataPtr ptr2 = std::make_shared<blockmirror::chain::Data>(
      blockmirror::chain::Data{"111", {1, 1, 1, 1}});

  blockmirror::chain::DataPtr ptr3 = std::make_shared<blockmirror::chain::Data>(
      blockmirror::chain::Data{"222", {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}});

  blockmirror::chain::DataPtr ptr4;

  {
    blockmirror::globalConfig.init("../config.json");
    //设置区块1
    block[1]->setGenesis();
    block[1]->setCoinbase(PUB(pubZhaoliu));
    block[1]->finalize(PRIV(privZhaoliu));

    blockmirror::chain::Context c;
    c.load();
    //执行区块
    BOOST_CHECK(c.apply(block[1]));
    BOOST_CHECK(c.check(ptr1));
    BOOST_CHECK(!c.check(ptr2));
    BOOST_CHECK(!c.check(ptr3));
    BOOST_CHECK(!c.check(ptr4));
    c.close();
  }

  {
    blockmirror::store::DataSignatureStore dataSignStore;
    dataSignStore.load(".");
    BOOST_CHECK(dataSignStore.query("111"));
  }
}

BOOST_AUTO_TEST_SUITE_END()