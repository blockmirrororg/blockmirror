#include <blockmirror/store/account_store.h>
#include <boost/test/unit_test.hpp>
#include "test_helper.h"

BOOST_AUTO_TEST_SUITE(account_tests)

BOOST_AUTO_TEST_CASE(account_tests_ok) {
  removeContextFiles();

  blockmirror::Pubkey pk1{1, 2, 3, 4, 5, 6, 7, 8};
  blockmirror::Pubkey pk2{1, 2, 3, 4, 5, 5};
  {
    blockmirror::store::AccountStore store;
    store.load(".");

    BOOST_CHECK_EQUAL(store.query(pk1), 0);
    BOOST_CHECK_EQUAL(store.query(pk2), 0);

    BOOST_CHECK(store.add(pk1, 1000));
    BOOST_CHECK(store.add(pk2, 100));
    BOOST_CHECK_EQUAL(store.query(pk1), 1000);
    BOOST_CHECK_EQUAL(store.query(pk2), 100);

    BOOST_CHECK(store.transfer(pk1, pk2, 900));
    BOOST_CHECK_EQUAL(store.query(pk2), 1000);
    BOOST_CHECK_EQUAL(store.query(pk1), 100);

    BOOST_CHECK(!store.transfer(pk1, pk2, 900));
  }
  {
    blockmirror::store::AccountStore store;
    store.load(".");
    BOOST_CHECK_EQUAL(store.query(pk2), 1000);
    BOOST_CHECK_EQUAL(store.query(pk1), 100);
  }
}

BOOST_AUTO_TEST_SUITE_END()