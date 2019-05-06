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
  }
  // 补充测试
}

BOOST_AUTO_TEST_SUITE_END()