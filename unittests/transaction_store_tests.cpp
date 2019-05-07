#include <blockmirror/common.h>
#include <blockmirror/store/transaction_store.h>
#include <boost/algorithm/hex.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(transaction_tests)

using namespace blockmirror;

template <typename Container>
void dump(const Container &container) {
  boost::algorithm::hex(container, std::ostream_iterator<char>(std::cout));
  std::cout << std::endl;
}

BOOST_AUTO_TEST_CASE(transaction_tests_ok) {
  auto trx1 = std::make_shared<chain::TransactionSigned>(
      chain::scri::Transfer({0}, 100));
  auto trx2 = std::make_shared<chain::TransactionSigned>(
      chain::scri::Transfer({0}, 1000));
  auto trx3 = std::make_shared<chain::TransactionSigned>(
      chain::scri::Transfer({0}, 10000));
  auto trx4 = std::make_shared<chain::TransactionSigned>(
      chain::scri::Transfer({0}, 100000));
  auto trxnon = std::make_shared<chain::TransactionSigned>(
      chain::scri::Transfer({0}, 1000000));
  
  trx1->setExpire(1);
  trx2->setExpire(2);
  trx3->setExpire(3);
  trx4->setExpire(4);

  boost::filesystem::remove("./transaction");
  {
    store::TransactionStore store;
    store.load(".");
    BOOST_CHECK(store.add(trx1));
    BOOST_CHECK(store.add(trx2, 1));
    BOOST_CHECK(store.add(trx3, 2));
    BOOST_CHECK(store.add(trx4));
    BOOST_CHECK(!store.add(trx1));
    BOOST_CHECK(!store.add(trx2, 1));
    BOOST_CHECK(!store.add(trx3, 2));
    BOOST_CHECK(!store.add(trx4));

    BOOST_CHECK(store.contains(trx1->getHashPtr()));
    BOOST_CHECK(store.contains(trx2->getHashPtr()));
    BOOST_CHECK(store.contains(trx3->getHashPtr()));
    BOOST_CHECK(store.contains(trx4->getHashPtr()));
    BOOST_CHECK(!store.contains(trxnon->getHashPtr()));

    auto hash4 = std::make_shared<Hash256>(trx4->getHash());
    BOOST_CHECK(store.contains(hash4));

    auto unpacked = store.popUnpacked();
    BOOST_CHECK(!store.contains(trx1->getHashPtr()));
    BOOST_CHECK(store.contains(trx2->getHashPtr()));
    BOOST_CHECK(store.contains(trx3->getHashPtr()));
    BOOST_CHECK(!store.contains(trx4->getHashPtr()));

    EqualTo equal;
    BOOST_CHECK_EQUAL(unpacked.size(), 2);
    for (auto i = unpacked.begin(); i != unpacked.end(); ++i) {
      BOOST_CHECK(equal((*i)->getHash(), trx1->getHash()) ||
                  equal((*i)->getHash(), trx4->getHash()));
    }
    BOOST_CHECK(store.add(trx1));
    BOOST_CHECK(store.add(trx4));

    store.removeExpired(2);
    BOOST_CHECK(!store.contains(trx1->getHashPtr()));
    BOOST_CHECK(!store.contains(trx2->getHashPtr()));
    BOOST_CHECK(store.contains(trx3->getHashPtr()));
    BOOST_CHECK(store.contains(trx4->getHashPtr()));

    BOOST_CHECK(!store.add(trx3, 5));
    BOOST_CHECK(!store.add(trx4, 2));
    unpacked = store.popUnpacked();
    BOOST_CHECK(unpacked.empty());

    BOOST_CHECK(!store.add(trx3));
    BOOST_CHECK(!store.add(trx4));
    unpacked = store.popUnpacked();
    BOOST_CHECK_EQUAL(unpacked.size(), 2);
    for (auto i = unpacked.begin(); i != unpacked.end(); ++i) {
      BOOST_CHECK(equal((*i)->getHash(), trx3->getHash()) ||
                  equal((*i)->getHash(), trx4->getHash()));
    }
    
    BOOST_CHECK(store.add(trx1));
    BOOST_CHECK(store.add(trx2, 1));
    BOOST_CHECK(store.add(trx3, 2));
    BOOST_CHECK(store.add(trx4));
  }
  {
    store::TransactionStore store;
    store.load(".");
    BOOST_CHECK(store.contains(trx1->getHashPtr()));
    BOOST_CHECK(store.contains(trx2->getHashPtr()));
    BOOST_CHECK(store.contains(trx3->getHashPtr()));
    BOOST_CHECK(store.contains(trx4->getHashPtr()));
    BOOST_CHECK(!store.contains(trxnon->getHashPtr()));
  }
}

BOOST_AUTO_TEST_SUITE_END()