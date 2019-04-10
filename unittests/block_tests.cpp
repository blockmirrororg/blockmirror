
#include <blockmirror/chain/block.h>
#include <blockmirror/serialization/binary_oarchive.h>
#include <boost/algorithm/hex.hpp>
#include <boost/test/unit_test.hpp>

#include <fstream>
#include <iostream>

BOOST_AUTO_TEST_SUITE(block_tests)

BOOST_AUTO_TEST_CASE(block_tests_serializer) {
  blockmirror::chain::BlockHeader header;
  header.height = 0x1122334455667788;
  header.merkleData.fill(0x11);
  header.merkleTransaction.fill(0x22);
  header.previous.fill(0x33);
  header.producer.fill(0x44);

  std::ostringstream oss;
  blockmirror::serialization::BinaryOarchive<std::ostringstream> oa(oss);
  oa << header;

  BOOST_CHECK_EQUAL(
      "887766554433221133333333333333333333333333333333333333333333333333333333"
      "333333332222222222222222222222222222222222222222222222222222222222222222"
      "111111111111111111111111111111111111111111111111111111111111111144444444"
      "4444444444444444444444444444444444444444444444444444444444",
      boost::algorithm::hex(oss.str()));
}

BOOST_AUTO_TEST_SUITE_END()