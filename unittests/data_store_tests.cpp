#include <blockmirror/store/data_store.h>
#include <boost/test/unit_test.hpp>
#include "test_helper.h"

BOOST_AUTO_TEST_SUITE(data_tests)

BOOST_AUTO_TEST_CASE(data_tests_ok) {
  removeContextFiles();

  blockmirror::store::NewDataPtr p0 =
    std::make_shared<blockmirror::chain::scri::NewData>(
      std::string{ "000" }, std::string{ "name" }, std::string{ "+++" });

  blockmirror::store::NewDataPtr p1 =
      std::make_shared<blockmirror::chain::scri::NewData>(
          std::string{"111"}, std::string{"abc"}, std::string{"+++"});

  blockmirror::store::NewDataPtr p2 =
      std::make_shared<blockmirror::chain::scri::NewData>(
          std::string{"abc"}, std::string{"222"}, std::string{"---"});

  // add将失败
  blockmirror::store::NewDataPtr p3 =
      std::make_shared<blockmirror::chain::scri::NewData>(
          std::string{"dddd"}, std::string{"abc"}, std::string{"dddddddddddd"});

  blockmirror::store::NewDataPtr p4 =
      std::make_shared<blockmirror::chain::scri::NewData>(
          std::string{"111"}, std::string{"kkkkk"}, std::string{"1111111111111"});

  //
  blockmirror::store::NewDataPtr p5 =
    std::make_shared<blockmirror::chain::scri::NewData>(
      std::string{ "111" }, std::string{ "name5" }, std::string{ "1111111111111" });

  blockmirror::store::NewDataPtr p6 =
    std::make_shared<blockmirror::chain::scri::NewData>(
      std::string{ "hello" }, std::string{ "name6" }, std::string{ "1111111111111" });

  blockmirror::store::NewDataPtr p7 =
    std::make_shared<blockmirror::chain::scri::NewData>(
      std::string{ "111" }, std::string{ "name7" }, std::string{ "1111111111111" });

  blockmirror::store::NewDataPtr p8 =
    std::make_shared<blockmirror::chain::scri::NewData>(
      std::string{ "world" }, std::string{ "name8" }, std::string{ "1111111111111" });

  blockmirror::store::NewDataPtr p9 =
    std::make_shared<blockmirror::chain::scri::NewData>(
      std::string{ "111" }, std::string{ "name9" }, std::string{ "1111111111111" });

  {
    blockmirror::store::DataStore store;
    store.load(".");

    BOOST_CHECK_EQUAL(store.query("111"), nullptr);
    BOOST_CHECK_EQUAL(store.query("abc"), nullptr);
    BOOST_CHECK_EQUAL(store.query("+++"), nullptr);
    BOOST_CHECK_EQUAL(store.query("222"), nullptr);

    BOOST_CHECK(store.add(p0));
    BOOST_CHECK(store.add(p1));
    BOOST_CHECK(store.add(p2));

    BOOST_CHECK(!store.add(p3)); // 期望是报错
    BOOST_CHECK(store.add(p4));

    BOOST_CHECK(store.add(p5));
    BOOST_CHECK(store.add(p6));
    BOOST_CHECK(store.add(p7));
    BOOST_CHECK(store.add(p8));
    BOOST_CHECK(store.add(p9));

    std::vector<blockmirror::store::NewDataPtr>& r = store.queryFormat("111");

    BOOST_CHECK_EQUAL(store.query("abc"), p1);
    BOOST_CHECK_EQUAL(store.query("222"), p2);
    BOOST_CHECK_EQUAL(store.query("---"), nullptr);
    BOOST_CHECK_EQUAL(store.query("111"), nullptr);

    const blockmirror::store::NewDataPtr p3 = store.query("abc");
    BOOST_CHECK_EQUAL(p3->getName(), "abc");
    BOOST_CHECK_EQUAL(p3->getFormat(), "111");
  }
  size_t filesize = 0;
  {
    blockmirror::store::DataStore store;
    store.load(".");

    BOOST_CHECK_EQUAL(store.query("111"), nullptr);
    BOOST_CHECK(store.query("abc"));
    BOOST_CHECK_EQUAL(store.query("+++"), nullptr);
    BOOST_CHECK(store.query("222"));

    BOOST_CHECK_EQUAL(store.add(p1), 0);
    BOOST_CHECK_EQUAL(store.add(p2), 0);

    BOOST_CHECK(store.query("abc"));
    BOOST_CHECK(store.query("222"));

    store.close();
    filesize = boost::filesystem::file_size("./data");
  }
  {
    size_t s = boost::filesystem::file_size("./data");
    BOOST_CHECK_EQUAL(filesize, s);

    blockmirror::store::DataStore store;
    store.load(".");

    BOOST_CHECK(store.query("abc"));
    BOOST_CHECK(store.query("222"));

    const blockmirror::store::NewDataPtr p3 = store.query("abc");
    BOOST_CHECK_EQUAL(p3->getName(), "abc");
    BOOST_CHECK_EQUAL(p3->getFormat(), "111");

    const blockmirror::store::NewDataPtr p5 = store.query("222");
    BOOST_CHECK_EQUAL(p5->getName(), "222");
    BOOST_CHECK_EQUAL(p5->getFormat(), "abc");

    store.close();
  }
}

BOOST_AUTO_TEST_SUITE_END()