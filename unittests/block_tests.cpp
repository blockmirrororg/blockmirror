
#include <blockmirror/chain/block.h>
#include <blockmirror/serialization/binary_oarchive.h>
#include <boost/algorithm/hex.hpp>
#include <boost/test/unit_test.hpp>

#include <fstream>
#include <iostream>

BOOST_AUTO_TEST_SUITE(block_tests)

BOOST_AUTO_TEST_CASE(block_tests_header) {
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
  auto &hash = header.getHash();
  BOOST_CHECK_EQUAL(
      boost::algorithm::hex(std::string(hash.begin(), hash.end())),
      "B6876325B061B9E68C7C7A449686314064C262CC076ADA8FEF43BCB2765149A6");

  blockmirror::Privkey priv = {0x12, 0xb0, 0x04, 0xff, 0xf7, 0xf4, 0xb6, 0x9e,
                               0xf8, 0x65, 0x0e, 0x76, 0x7f, 0x18, 0xf1, 0x1e,
                               0xde, 0x15, 0x81, 0x48, 0xb4, 0x25, 0x66, 0x07,
                               0x23, 0xb9, 0xf9, 0xa6, 0x6e, 0x61, 0xf7, 0x47};
  blockmirror::Pubkey pub = {
      0x03, 0x0b, 0x4c, 0x86, 0x65, 0x85, 0xdd, 0x86, 0x8a, 0x9d, 0x62,
      0x34, 0x8a, 0x9c, 0xd0, 0x08, 0xd6, 0xa3, 0x12, 0x93, 0x70, 0x48,
      0xff, 0xf3, 0x16, 0x70, 0xe7, 0xe9, 0x20, 0xcf, 0xc7, 0xa7, 0x44};
  blockmirror::chain::BlockHeaderSigned headerSigned;
  headerSigned.height = 0x1122334455667788;
  headerSigned.merkleData.fill(0x11);
  headerSigned.merkleTransaction.fill(0x22);
  headerSigned.previous.fill(0x33);

  headerSigned.sign(priv);

  auto &hash2 = headerSigned.getHash();
  BOOST_CHECK_EQUAL(
      boost::algorithm::hex(std::string(hash2.begin(), hash2.end())),
      "36C46AC5015D50D06111FC7245B57353AD43C82B617C39F74524083D3CB5F5FC");

  BOOST_CHECK_EQUAL_COLLECTIONS(pub.begin(), pub.end(),
                                headerSigned.producer.begin(),
                                headerSigned.producer.end());

  BOOST_CHECK(headerSigned.verify());

  oss.clear();
  oa << headerSigned;

  BOOST_CHECK_EQUAL(
      "887766554433221133333333333333333333333333333333333333333333333333333333"
      "333333332222222222222222222222222222222222222222222222222222222222222222"
      "111111111111111111111111111111111111111111111111111111111111111144444444"
      "444444444444444444444444444444444444444444444444444444444488776655443322"
      "113333333333333333333333333333333333333333333333333333333333333333222222"
      "222222222222222222222222222222222222222222222222222222222211111111111111"
      "11111111111111111111111111111111111111111111111111030B4C866585DD868A9D62"
      "348A9CD008D6A312937048FFF31670E7E920CFC7A744304402205D5AA091775536B58FB9"
      "89DC2D1B8788E641AEA16B7D6A8EAE7C9992B5B0AB2602200795F8D2B1188934CD8E0734"
      "DA3B68E5C13A5965723B46F746278640551F25200010",
      boost::algorithm::hex(oss.str()));
}

BOOST_AUTO_TEST_CASE(block_tests_transactions) {}

BOOST_AUTO_TEST_SUITE_END()