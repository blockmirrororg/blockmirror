#include <blockmirror/store/bps_store.h>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(bps_tests)

BOOST_AUTO_TEST_CASE(bps_tests_ok) {
  blockmirror::Pubkey pk1{1, 2, 3, 4, 5, 6, 7, 8};
  blockmirror::Pubkey pk2{1, 2, 3, 4, 5, 5};
  boost::filesystem::remove("./bps");
  {
    blockmirror::store::BpsStore store;
    store.load(".");

    BOOST_CHECK_EQUAL(store.contains(pk1),0);
    BOOST_CHECK_EQUAL(store.contains(pk2),0);
    BOOST_CHECK_EQUAL(store.remove(pk1),0);
    BOOST_CHECK_EQUAL(store.remove(pk2),0);

    BOOST_CHECK(store.add(pk1));
    BOOST_CHECK(store.add(pk2));
    BOOST_CHECK_EQUAL(store.contains(pk1),1);
    BOOST_CHECK_EQUAL(store.contains(pk2),1);

    BOOST_CHECK_EQUAL(store.remove(pk1),1);
    BOOST_CHECK_EQUAL(store.remove(pk2),1);
    BOOST_CHECK_EQUAL(store.contains(pk1),0);
    BOOST_CHECK_EQUAL(store.contains(pk2),0);
  }
  size_t filesize = 0;
  {
    blockmirror::store::BpsStore store;
    store.load(".");

    BOOST_CHECK_EQUAL(store.contains(pk1),0);
    BOOST_CHECK_EQUAL(store.contains(pk2),0);

    BOOST_CHECK(store.add(pk1));
    BOOST_CHECK(store.add(pk2));

    store.close();
    filesize = boost::filesystem::file_size("./bps");
  }
  {
    size_t s = boost::filesystem::file_size("./bps");
    BOOST_CHECK_EQUAL(filesize,s);

    blockmirror::store::BpsStore store;
    store.load(".");

    BOOST_CHECK(store.contains(pk1));
    BOOST_CHECK(store.contains(pk2));

    BOOST_CHECK_EQUAL(store.remove(pk1),1);

    BOOST_CHECK_EQUAL(store.contains(pk1),0);
    BOOST_CHECK_EQUAL(store.contains(pk2),1);
  }
  {
    blockmirror::store::BpsStore store;
    store.load(".");

    BOOST_CHECK_EQUAL(store.contains(pk1),0);
    BOOST_CHECK_EQUAL(store.contains(pk2),1);
    BOOST_CHECK_EQUAL(store.add(pk2),0);

    BOOST_CHECK_EQUAL(store.remove(pk1),0);
    BOOST_CHECK_EQUAL(store.remove(pk2),1);

    BOOST_CHECK_EQUAL(store.add(pk1),1);
    BOOST_CHECK_EQUAL(store.add(pk2),1);
  }

}

BOOST_AUTO_TEST_SUITE_END()