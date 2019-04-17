// 测试列表:
// 1. 无符号整数
// 2. 带符号整数
// 3. 单精度和双精度浮点是IEEE754编码
// 4. std::array<uint8_t, X>
// 5. std::vector<uint8_t>
// 6. std::string
// 7. std::vector<T>
// 8. boost::variant
#include <blockmirror/chain/data.h>
#include <blockmirror/serialization/binary_iarchive.h>
#include <blockmirror/serialization/binary_oarchive.h>
#include <blockmirror/serialization/json_oarchive.h>
#include <blockmirror/serialization/ptree_iarchive.h>
#include <boost/algorithm/hex.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/test/unit_test.hpp>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

template <typename T>
std::string S(const T &value) {
  std::ostringstream oss;
  blockmirror::serialization::BinaryOArchive<std::ostringstream> archive(oss);
  archive << value;
  return boost::algorithm::hex(oss.str());
}
template <typename T>
std::string J(const T &value) {
  std::ostringstream oss;
  blockmirror::serialization::JSONOArchive<std::ostringstream> archive(oss,
                                                                       false);
  archive << value;
  return oss.str();
}
template <typename T>
void IJ(const std::string &value, T &out) {
  std::stringstream ss(value);
  boost::property_tree::ptree ptree;
  boost::property_tree::read_json(ss, ptree);
  blockmirror::serialization::PTreeIArchive archive(ptree);
  archive >> out;
}
template <typename T>
void IS(const std::string &value, T &out) {
  std::istringstream iss(boost::algorithm::unhex(value));
  blockmirror::serialization::BinaryIArchive<std::istringstream> archive(iss);
  archive >> out;
}

template <typename T>
bool operator==(const std::vector<T> &a, const std::vector<T> &b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); i++) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

BOOST_AUTO_TEST_SUITE(serialization_tests)

template <typename T>
void checkValue(const T &value, const char *str, const char *json) {
  std::string ostr = S(value);
  BOOST_CHECK_EQUAL(ostr, str);
  T ovalue;
  IS(ostr, ovalue);
  BOOST_CHECK(ovalue == value);

  std::string jstr = J(value);
  BOOST_CHECK_EQUAL(jstr, json);
  T oovalue;
  IJ(jstr, oovalue);
  BOOST_CHECK(oovalue == value);
}

void checkUInt(uint64_t value, const char *str, const char *json) {
  std::string ostr;
  if (std::numeric_limits<uint16_t>::max() >= value) {
    checkValue((uint16_t)value, str, json);
  }
  if (std::numeric_limits<uint32_t>::max() >= value) {
    checkValue((uint32_t)value, str, json);
  }
  checkValue((uint64_t)value, str, json);
}

BOOST_AUTO_TEST_CASE(serialization_tests_bin_integer) {
  BOOST_CHECK_EQUAL(S((uint8_t)0xFF), "FF");
  BOOST_CHECK_EQUAL(S((int8_t)-1), "FF");

  checkUInt(0ull, "00", "0");
  checkUInt(0x80ull, "8001", "128");
  checkUInt(0x3FFFull, "FF7F", "16383");
  checkUInt(0x4000ull, "808001", "16384");
  checkUInt(0xFFFFull, "FFFF03", "65535");
  checkUInt(0x1FFFFFull, "FFFF7F", "2097151");
  checkUInt(0x200000ull, "80808001", "2097152");
  checkUInt(0xFFFFFFFull, "FFFFFF7F", "268435455");
  checkUInt(0x10000000ull, "8080808001", "268435456");
  checkUInt(0xFFFFFFFFull, "FFFFFFFF0F", "4294967295");
  checkUInt(0xFFFFFFFFFFFFFFFFull, "FFFFFFFFFFFFFFFFFF01",
            "18446744073709551615");
}

BOOST_AUTO_TEST_CASE(serialization_tests_binary) {
  checkValue(std::vector<uint8_t>{1, 2, 3, 4, 5}, "050102030405",
             "\"0102030405\"");
  checkValue(std::array<uint8_t, 5>{1, 2, 3, 4, 5}, "0102030405",
             "\"0102030405\"");
  // array
  checkValue(std::vector<uint16_t>{1, 2, 3, 4, 5}, "050102030405",
             "[ 1, 2, 3, 4, 5]");
}

BOOST_AUTO_TEST_CASE(serialization_tests_variant) {
  using VariantType =
      boost::variant<unsigned int, float, std::string, int,
                     std::vector<uint8_t>, std::array<uint8_t, 4>>;
  checkValue(VariantType((unsigned int)0x11), "0011",
             "{\"type\": 0, \"value\": 17}");
  checkValue(VariantType((float)1.0f), "010000803F",
             "{\"type\": 1, \"value\": 1}");
  checkValue(VariantType(std::string("0000")), "020430303030",
             "{\"type\": 2, \"value\": \"0000\"}");
  checkValue(VariantType((int)-0x11), "0323", "{\"type\": 3, \"value\": -17}");
  checkValue(VariantType(std::vector<uint8_t>{8, 6, 4, 2}), "040408060402",
             "{\"type\": 4, \"value\": \"08060402\"}");
  checkValue(VariantType(std::array<uint8_t, 4>{8, 6, 4, 2}), "0508060402",
             "{\"type\": 5, \"value\": \"08060402\"}");
}

BOOST_AUTO_TEST_SUITE_END()