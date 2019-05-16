#include <blockmirror/chain/context.h>
#include <blockmirror/common.h>
#include <blockmirror/store/account_store.h>
#include <blockmirror/store/bps_store.h>
#include <blockmirror/store/data_store.h>
#include <blockmirror/store/format_store.h>
#include <blockmirror/store/transaction_store.h>
#include <boost/algorithm/hex.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(context_tests)

blockmirror::Privkey K(const std::string& str) {
  blockmirror::Privkey priv;
  boost::algorithm::unhex(str, priv.begin());
  return priv;
}

blockmirror::Pubkey P(const std::string& str) {
  blockmirror::Pubkey pub;
  boost::algorithm::unhex(str, pub.begin());
  return pub;
}

std::string APub =
    "0349622F329912F575DB5E1FC15849CA78DD0A2DD0EEBF34D2FD9683A2C2B3B924";
std::string APriv =
    "32CD6A9C3B4D58C06606513A7C630307C3E08A42599C54BDB17D5C81EC847B9E";
std::string BPub =
    "0213E21D6D3A4D64994E938F51A128861DEA7395A456C08F62A4549DF904D4B525";
std::string BPriv =
    "95D4B0DF5B1069B47F35C8C7A6764BB8B760D0359B6C1221DDCB46CE5830E14C";

//生成公私钥
std::vector<blockmirror::Privkey> pris;
std::vector<blockmirror::Pubkey> pubs;
void createKey() {
  pris.resize(10);
  pubs.resize(10);
  for (size_t i = 0; i < pris.size(); i++) {
    blockmirror::crypto::ECC.newKey(pris[i]);
    blockmirror::crypto::ECC.computePub(pris[i], pubs[i]);
  }
}

//生成区块
blockmirror::chain::BlockPtr block[10];
void createBlock() {
  for (auto i = 0; i < 10; i++) {
    block[i] = std::make_shared<blockmirror::chain::Block>();
  }
}

//生成各种交易
blockmirror::chain::TransactionSignedPtr tPtr[10]{
    std::make_shared<blockmirror::chain::TransactionSigned>(
        blockmirror::chain::scri::Transfer(P(BPub), 100)),  // 0

    std::make_shared<blockmirror::chain::TransactionSigned>(
        blockmirror::chain::scri::BPJoin({1,2,3})),  // 1

    std::make_shared<blockmirror::chain::TransactionSigned>(
        blockmirror::chain::scri::BPJoin({4,5,6})),  // 2

    std::make_shared<blockmirror::chain::TransactionSigned>(
        blockmirror::chain::scri::BPExit({4,5,6})),  // 3

    std::make_shared<blockmirror::chain::TransactionSigned>(
        blockmirror::chain::scri::NewData("aaa", "111", "rrt")),  // 4

    std::make_shared<blockmirror::chain::TransactionSigned>(
        blockmirror::chain::scri::NewFormat("aaa", "ggg", {1, 3, 5, 7, 9},
                                            {2, 4, 6, 8}, {6, 6, 6})),  // 5

    std::make_shared<blockmirror::chain::TransactionSigned>(
        blockmirror::chain::scri::BPJoin({4,5,6})),  // 6

    std::make_shared<blockmirror::chain::TransactionSigned>(
        blockmirror::chain::scri::NewFormat("bbb", "ccc", {15, 3, 55, 75, 9},
                                            {28, 4, 65, 8}, {60, 76, 6})),  // 7

    std::make_shared<blockmirror::chain::TransactionSigned>(
        blockmirror::chain::scri::NewFormat("aaa2", "fff", {12, 13, 15, 27, 9},
                                            {2, 45, 65, 85},
                                            {96, 61, 126}))};  // 8

void removeFile() {
  boost::filesystem::remove("./0.block");
  boost::filesystem::remove("./index");
  boost::filesystem::remove("./account");
  boost::filesystem::remove("./bps");
  boost::filesystem::remove("./data");
  boost::filesystem::remove("./format");
  boost::filesystem::remove("./transaction");
  boost::filesystem::remove("./head");
}

void saveBlock(int b, int e) {
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
  removeFile();
  createKey();
  createBlock();

  //设置区块1
  block[1]->setGenesis();
  block[1]->setCoinbase(P(APub));
  block[1]->finalize(K(APriv));

  //设置区块2
  block[2]->setPrevious(*block[1]);
  block[2]->setCoinbase(P(APub));
  // 两个BP 下一个时间时轮到 B 出
  // 所以要区块2必须 是下下一个时间 才轮到A
  block[2]->setTimestamp(block[1]->getTimestamp() + 2 * blockmirror::BLOCK_PER_MS);
  block[2]->finalize(K(APriv));

  //设置区块2的交易
  for (size_t i = 0; i < 6; i++) {
    if (i == 0) {
      tPtr[i]->addSign(K(APriv));  //这里是A转账给B
    } else {
      tPtr[i]->addSign(pris[i]);
    }

    block[2]->addTransaction(tPtr[i]);

    B_LOG("{}", spdlog::to_hex(tPtr[i]->getHash()));
  }

  // 设置区块3
  block[3]->setPrevious(*block[2]);
  block[3]->setCoinbase(P(BPub));
  block[3]->setTimestamp(block[2]->getTimestamp() + 2 * blockmirror::BLOCK_PER_MS);
  block[3]->finalize(K(BPriv));

  //设置区块3的交易
  for (size_t i = 6; i < 9; i++) {
    tPtr[i]->addSign(pris[i]);
    block[3]->addTransaction(tPtr[i]);
  }
  // 增加的BP列表
  {
    blockmirror::store::BpsStore store;
    store.load(".");
    BOOST_CHECK(store.add(block[1]->getProducer()));
    BOOST_CHECK(store.add(block[3]->getProducer()));
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
    BOOST_CHECK_EQUAL(accountStore.query(P(APub)), 199999900);
    BOOST_CHECK_EQUAL(accountStore.query(P(BPub)), 100000100);

    blockmirror::store::BpsStore bpsStore;
    bpsStore.load(".");
    BOOST_CHECK_EQUAL(bpsStore.contains(P(APub)), 1);
    BOOST_CHECK_EQUAL(bpsStore.contains(P(BPub)), 1);

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
    BOOST_CHECK_EQUAL(store.query(P(APub)), 0);
    BOOST_CHECK_EQUAL(store.query(P(BPub)), 0);

    blockmirror::store::BpsStore bpsStore;
    bpsStore.load(".");
    BOOST_CHECK_EQUAL(bpsStore.contains({1,2,3}), 0);
    BOOST_CHECK_EQUAL(bpsStore.contains({4,5,6}), 0);

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
  removeFile();
  createKey();
  createBlock();

  //设置区块4
  block[4]->setGenesis();
  block[4]->setCoinbase(P(BPub));
  block[4]->finalize(K(BPriv));

  //设置区块4的交易
  tPtr[7]->addSign(K(BPriv));
  block[4]->addTransaction(tPtr[7]);

  //设置区块5
  block[5]->setPrevious(*block[4]);
  block[5]->setCoinbase(P(BPub));
  block[5]->finalize(K(BPriv));

  //设置区块5的交易
  tPtr[4]->addSign(K(BPriv));
  block[5]->addTransaction(tPtr[4]);

  tPtr[4]->addSign(
      K(APriv));  //设置相同的NewData，导致这个区块执行交易回退，同时head回退
  block[5]->addTransaction(tPtr[4]);

  //设置区块6
  block[6]->setPrevious(*block[5]);
  block[6]->setCoinbase(P(BPub));
  block[6]->finalize(K(BPriv));

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
  removeFile();
  createKey();
  createBlock();

  //设置区块4
  block[4]->setGenesis();
  block[4]->setCoinbase(P(BPub));
  block[4]->finalize(K(BPriv));

  //设置区块4的交易
  tPtr[7]->addSign(pris[1]);
  block[4]->addTransaction(tPtr[7]);

  tPtr[0]->addSign(K(APriv));  // A给B转账，但是A余额不足
  block[4]->addTransaction(tPtr[0]);

  {  //执行区块4
    blockmirror::chain::Context c;
    c.load();
    BOOST_CHECK(!c.apply(block[4]));
    c.close();
  }

  blockmirror::store::AccountStore store;
  store.load(".");
  // A虽然给B转账100，但是失败，coinbase也进行了回滚，所以B余额是0
  BOOST_CHECK_EQUAL(store.query(P(BPub)), 0);

  blockmirror::store::TransactionStore transactionStore;
  transactionStore.load(".");
  BOOST_CHECK_EQUAL(transactionStore.contains(tPtr[7]->getHashPtr()), 1);
  // tPtr[7]的NewFormat因为tPtr[0]失败，tPtr[7]进行了回滚，并没有保存
  blockmirror::store::FormatStore formatStore;
  formatStore.load(".");
  BOOST_CHECK_EQUAL(formatStore.query("bbb"), nullptr);
}

BOOST_AUTO_TEST_CASE(context_tests_ok4) {
  removeFile();
  createKey();
  createBlock();

  {  //还没执行过区块，回滚失败
    blockmirror::chain::Context c;
    c.load();
    BOOST_CHECK(!c.rollback());
    c.close();
  }

  //设置区块4
  block[4]->setGenesis();
  block[4]->setCoinbase(P(BPub));
  block[4]->finalize(K(BPriv));

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
  // 回滚失败，所以B余额是coinbase
  BOOST_CHECK_EQUAL(store.query(P(BPub)), 100000000);

  blockmirror::store::BpsStore bpsStore;
  bpsStore.load(".");
  BOOST_CHECK_EQUAL(bpsStore.contains(P(BPub)), 1);  //回滚失败，B仍然还是BP

  blockmirror::store::TransactionStore transactionStore;
  transactionStore.load(".");
  BOOST_CHECK_EQUAL(transactionStore.contains(tPtr[7]->getHashPtr()), 1);
}

BOOST_AUTO_TEST_CASE(context_tests_ok5) {
  removeFile();
  createKey();
  createBlock();

  //设置区块5
  block[5]->setGenesis();
  block[5]->setCoinbase(P(BPub));
  block[5]->finalize(K(BPriv));

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
  removeFile();
  createKey();
  createBlock();

  //设置区块5
  block[5]->setGenesis();
  block[5]->setCoinbase(P(BPub), 100);  //不等于MINER_AMOUNT
  block[5]->finalize(K(BPriv));

  //设置区块4
  block[4]->setGenesis();
  block[4]->setCoinbase(P(BPub));
  block[4]->finalize(K(BPriv));

  //设置区块6
  block[6]->setPrevious(*block[5]);
  block[6]->setCoinbase(P(APub));
  block[6]->finalize(K(APriv));

  {  //先执行区块6,再执行区块5
    blockmirror::chain::Context c;
    c.load();
    BOOST_CHECK(!c.apply(block[6]));
    BOOST_CHECK(!c.apply(block[5]));
    c.close();
  }
}

BOOST_AUTO_TEST_CASE(context_tests_ok7) {
  removeFile();
  createKey();
  createBlock();
}

BOOST_AUTO_TEST_SUITE_END()