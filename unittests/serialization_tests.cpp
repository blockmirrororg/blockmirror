// 测试列表:
// 1. 无符号整数
// 2. 带符号整数
// 3. 单精度和双精度浮点是IEEE754编码
// 4. std::array<uint8_t, X>
// 5. std::vector<uint8_t>
// 6. std::string
// 7. std::vector<T>
// 8. boost::variant
// 9. chain 的各种结构
#include <blockmirror/chain/data.h>
#include <blockmirror/serialization/binary_oarchive.h>
#include <blockmirror/serialization/json_oarchive.h>
#include <boost/algorithm/hex.hpp>
#include <boost/test/unit_test.hpp>

#include <fstream>
#include <iostream>

template <typename T>
std::string S(const T &value) {
  std::ostringstream oss;
  blockmirror::serialization::BinaryOarchive<std::ostringstream> archive(oss);
  archive << value;
  return boost::algorithm::hex(oss.str());
}
template <typename T>
std::string J(const T &value) {
  std::ostringstream oss;
  blockmirror::serialization::JSONOarchive<std::ostringstream> archive(oss);
  archive << value;
  return oss.str();
}

BOOST_AUTO_TEST_SUITE(serialization_tests)

void checkUInt(uint64_t value, const char *str) {
  if (std::numeric_limits<uint16_t>::max() >= value) {
    BOOST_CHECK_EQUAL(S((uint16_t)value), str);
  }
  if (std::numeric_limits<uint32_t>::max() >= value) {
    BOOST_CHECK_EQUAL(S((uint32_t)value), str);
  }
  BOOST_CHECK_EQUAL(S(value), str);
}

BOOST_AUTO_TEST_CASE(serialization_tests_bin_integer) {
  BOOST_CHECK_EQUAL(S((uint8_t)0xFF), "FF");
  BOOST_CHECK_EQUAL(S((int8_t)-1), "FF");

  checkUInt(0ull, "00");
  checkUInt(0x80ull, "8001");
  checkUInt(0x3FFFull, "FF7F");
  checkUInt(0x4000ull, "808001");
  checkUInt(0xFFFFull, "FFFF03");
  checkUInt(0x1FFFFFull, "FFFF7F");
  checkUInt(0x200000ull, "80808001");
  checkUInt(0xFFFFFFFull, "FFFFFF7F");
  checkUInt(0x10000000ull, "8080808001");
  checkUInt(0xFFFFFFFFull, "FFFFFFFF0F");
  checkUInt(0xFFFFFFFFFFFFFFFFull, "FFFFFFFFFFFFFFFFFF01");
}

BOOST_AUTO_TEST_CASE(serialization_tests_bin_data) {
  BOOST_CHECK_EQUAL(S(blockmirror::chain::Data("Hello", {1, 2, 3})), "0548656C6C6F03010203");
}

BOOST_AUTO_TEST_SUITE_END()