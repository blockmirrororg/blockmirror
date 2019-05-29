#include <blockmirror/store/format_store.h>
#include <boost/test/unit_test.hpp>
#include "test_helper.h"

BOOST_AUTO_TEST_SUITE(format_tests)

BOOST_AUTO_TEST_CASE(format_tests_ok) {
  removeContextFiles();

  std::vector<uint8_t> v1{1, 2, 3};
  blockmirror::store::NewFormatPtr p1 =
      std::make_shared<blockmirror::chain::scri::NewFormat>(
          std::string{"111"}, std::string{"222"}, v1, v1, v1);

  std::vector<uint8_t> v2{5, 5, 5};
  blockmirror::store::NewFormatPtr p2 =
      std::make_shared<blockmirror::chain::scri::NewFormat>(
          std::string{"aaa"}, std::string{"ccc"}, v2, v2, v2);

  {
    blockmirror::store::FormatStore store;
    store.load(".");

    BOOST_CHECK(!store.query("111"));
    BOOST_CHECK(!store.query("222"));
    BOOST_CHECK(!store.query("aaa"));

    BOOST_CHECK(store.add(p1));
    BOOST_CHECK(store.add(p2));

    BOOST_CHECK(store.query("111"));
    BOOST_CHECK(!store.query("222"));
    BOOST_CHECK(store.query("aaa"));
  }
  size_t filesize = 0;
  {
    blockmirror::store::FormatStore store;
    store.load("./");

    BOOST_CHECK(store.query("111"));
    BOOST_CHECK(store.query("aaa"));

    BOOST_CHECK_EQUAL(store.add(p1), 0);
    BOOST_CHECK_EQUAL(store.add(p2), 0);

    store.close();

    filesize = boost::filesystem::file_size("./format");
  }
  {
    size_t s = boost::filesystem::file_size("./format");
    BOOST_CHECK_EQUAL(filesize, s);

    blockmirror::store::FormatStore store;
    store.load(".");

    BOOST_CHECK(store.query("111"));
    BOOST_CHECK(store.query("aaa"));

    blockmirror::store::NewFormatPtr p3 = store.query("111");
    BOOST_CHECK_EQUAL(p3->getName(), "111");
    BOOST_CHECK_EQUAL(p3->getDesc(), "222");
    BOOST_CHECK(p3->getDataFormat() == v1);
    BOOST_CHECK(p3->getResultScript() == v1);
    BOOST_CHECK(p3->getValidScript() == v1);

    blockmirror::store::NewFormatPtr p5 = store.query("aaa");
    BOOST_CHECK_EQUAL(p5->getName(), "aaa");
    BOOST_CHECK_EQUAL(p5->getDesc(), "ccc");
    BOOST_CHECK(p5->getDataFormat() == v2);
    BOOST_CHECK(p5->getResultScript() == v2);
    BOOST_CHECK(p5->getValidScript() == v2);
  }
}

BOOST_AUTO_TEST_SUITE_END()