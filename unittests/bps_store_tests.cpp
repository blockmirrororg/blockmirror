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

    BOOST_CHECK_EQUAL(store.contains(pk1), 0);
    BOOST_CHECK_EQUAL(store.contains(pk2), 0);
    BOOST_CHECK_EQUAL(store.remove(pk1), 0);
    BOOST_CHECK_EQUAL(store.remove(pk2), 0);

    BOOST_CHECK(store.add(pk1));
    BOOST_CHECK(store.add(pk2));
    BOOST_CHECK_EQUAL(store.contains(pk1), 1);
    BOOST_CHECK_EQUAL(store.contains(pk2), 1);

    BOOST_CHECK_EQUAL(store.remove(pk1), 1);
    BOOST_CHECK_EQUAL(store.remove(pk2), 1);
    BOOST_CHECK_EQUAL(store.contains(pk1), 0);
    BOOST_CHECK_EQUAL(store.contains(pk2), 0);
  }
  size_t filesize = 0;
  {
    blockmirror::store::BpsStore store;
    store.load(".");

    BOOST_CHECK_EQUAL(store.contains(pk1), 0);
    BOOST_CHECK_EQUAL(store.contains(pk2), 0);

    BOOST_CHECK(store.add(pk1));
    BOOST_CHECK(store.add(pk2));

    store.close();
    filesize = boost::filesystem::file_size("./bps");
  }
  {
    size_t s = boost::filesystem::file_size("./bps");
    BOOST_CHECK_EQUAL(filesize, s);

    blockmirror::store::BpsStore store;
    store.load(".");

    BOOST_CHECK(store.contains(pk1));
    BOOST_CHECK(store.contains(pk2));

    BOOST_CHECK_EQUAL(store.remove(pk1), 1);

    BOOST_CHECK_EQUAL(store.contains(pk1), 0);
    BOOST_CHECK_EQUAL(store.contains(pk2), 1);
  }
  {
    blockmirror::store::BpsStore store;
    store.load(".");

    BOOST_CHECK_EQUAL(store.contains(pk1), 0);
    BOOST_CHECK_EQUAL(store.contains(pk2), 1);
    BOOST_CHECK_EQUAL(store.add(pk2), 0);

    BOOST_CHECK_EQUAL(store.remove(pk1), 0);
    BOOST_CHECK_EQUAL(store.remove(pk2), 1);

    BOOST_CHECK_EQUAL(store.add(pk1), 1);
    BOOST_CHECK_EQUAL(store.add(pk2), 1);
  }
}

BOOST_AUTO_TEST_CASE(bps_tests_producerTime) {
  using namespace blockmirror;
  Pubkey pk1{1, 2, 3, 4, 5, 6, 7, 8};
  Pubkey pk2{1, 2, 3, 4, 5, 5};
  Pubkey pk3{1, 2};
  boost::filesystem::remove("./bps");
  auto now = (now_ms_since_1970() / BLOCK_PER_MS) * BLOCK_PER_MS;
  {
    store::BpsStore store;
    store.load(".");
    store.add(pk1);
    store.add(pk2);
    store.pushBpChange(0, now);
  }
  {
    store::BpsStore store;
    store.load(".");
    BOOST_CHECK_EQUAL(store.getBPDelay(pk2, now), 1000);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk1, now), 0);
    // 1毫秒后
    BOOST_CHECK_EQUAL(store.getBPDelay(pk2, now + 1), 999);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk1, now + 1), 1999);
    // 半秒后
    BOOST_CHECK_EQUAL(store.getBPDelay(pk2, now + 500), 500);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk1, now + 500), 1500);
    // 一秒后
    BOOST_CHECK_EQUAL(store.getBPDelay(pk2, now + 1000), 0);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk1, now + 1000), 1000);
    // 1.5秒后
    BOOST_CHECK_EQUAL(store.getBPDelay(pk2, now + 1500), 1500);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk1, now + 1500), 500);
    // 2秒后
    BOOST_CHECK_EQUAL(store.getBPDelay(pk2, now + 2000), 1000);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk1, now + 2000), 0);
    // 2.5秒后
    BOOST_CHECK_EQUAL(store.getBPDelay(pk2, now + 2500), 500);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk1, now + 2500), 1500);
  }
  {
    store::BpsStore store;
    store.load(".");
    store.add(pk3);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk3, now), 2000);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk2, now), 1000);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk1, now), 0);
    // 1毫秒后
    BOOST_CHECK_EQUAL(store.getBPDelay(pk3, now + 1), 1999);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk2, now + 1), 999);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk1, now + 1), 2999);
    // 500毫秒后
    BOOST_CHECK_EQUAL(store.getBPDelay(pk3, now + 500), 1500);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk2, now + 500), 500);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk1, now + 500), 2500);
    // 1秒后
    BOOST_CHECK_EQUAL(store.getBPDelay(pk3, now + 1000), 1000);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk2, now + 1000), 0);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk1, now + 1000), 2000);
    // 1.5秒后
    BOOST_CHECK_EQUAL(store.getBPDelay(pk3, now + 1500), 500);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk2, now + 1500), 2500);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk1, now + 1500), 1500);
    // 2秒后
    BOOST_CHECK_EQUAL(store.getBPDelay(pk3, now + 2000), 0);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk2, now + 2000), 2000);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk1, now + 2000), 1000);
    // 2.5秒后
    BOOST_CHECK_EQUAL(store.getBPDelay(pk3, now + 2500), 2500);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk2, now + 2500), 1500);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk1, now + 2500), 500);
    // 3秒后
    BOOST_CHECK_EQUAL(store.getBPDelay(pk3, now + 3000), 2000);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk2, now + 3000), 1000);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk1, now + 3000), 0);
    // 3.5秒后
    BOOST_CHECK_EQUAL(store.getBPDelay(pk3, now + 3500), 1500);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk2, now + 3500), 500);
    BOOST_CHECK_EQUAL(store.getBPDelay(pk1, now + 3500), 2500);
  }
}

BOOST_AUTO_TEST_SUITE_END()