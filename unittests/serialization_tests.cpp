// 测试列表:
// 1. 无符号整数
// 2. 带符号整数
// 3. 单精度和双精度 IEEE754编码
// 4. std::array<uint8_t, X>
// 5. std::vector<uint8_t>
// 6. std::string
// 7. std::vector<T>
// 8. boost::variant
#include <blockmirror/chain/block.h>
#include <blockmirror/chain/data.h>
#include <blockmirror/serialization/binary_iarchive.h>
#include <blockmirror/serialization/binary_oarchive.h>
#include <blockmirror/serialization/bson_oarchive.h>
#include <blockmirror/serialization/json_oarchive.h>
#include <blockmirror/serialization/ptree_iarchive.h>
#include <boost/property_tree/json_parser.hpp>
#include <boost/test/unit_test.hpp>
#include <bsoncxx/json.hpp>
#include "test_helper.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

struct TTTClass {
  friend class blockmirror::serialization::access;
  template <typename Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(val);
  }
  int val;
  TTTClass() : val(111) {}
};

using VariantType =
    boost::variant<unsigned int, float, std::string, int, std::vector<uint8_t>,
                   std::array<uint8_t, 4>, std::shared_ptr<int>>;
using VariantType2 =
    boost::variant<unsigned int, float, std::string, int, std::vector<uint8_t>,
                   std::array<uint8_t, 4>, std::shared_ptr<int>, TTTClass>;

template <typename T>
static inline bool operator==(const std::vector<T> &a,
                              const std::vector<T> &b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); i++) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

BOOST_AUTO_TEST_SUITE(serialization_tests)

template <typename T>
void checkValue(const T &value, const char *str, const char *json,
                bool noCheckValue = false) {
  std::string ostr = SerOToHex(value);
  BOOST_CHECK_EQUAL(ostr, str);
  T ovalue;
  SerOFromHex(ostr, ovalue);
  if (!noCheckValue) {
    BOOST_CHECK(ovalue == value);
  }

  std::string jstr = SerOToJson(value, false);
  BOOST_CHECK_EQUAL(jstr, json);
  T oovalue;
  SerOFromJSON(jstr, oovalue);
  if (!noCheckValue) {
    BOOST_CHECK(oovalue == value);
  }
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
  BOOST_CHECK_EQUAL(SerOToHex((uint8_t)0xFF), "FF");
  BOOST_CHECK_EQUAL(SerOToHex((int8_t)-1), "FF");

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
  checkValue(VariantType(std::make_shared<int>(0x11)), "0622",
             "{\"type\": 6, \"value\": 17}", true);
}

template <typename T>
std::string toBSONStr(const std::string &name, T &val) {
  blockmirror::chain::Context empty;
  bsoncxx::builder::stream::document doc;
  blockmirror::serialization::BSONOArchive<bsoncxx::builder::stream::document>
      archive(empty, doc);
  archive << boost::serialization::make_nvp(name.c_str(), val);
  std::string json = bsoncxx::to_json(doc.view());
  std::string result;
  for (auto ch : json) {
    switch (ch) {
      case ' ':
      case '\t':
      case '\r':
      case '\n':
        break;
      default:
        result.push_back(ch);
        break;
    }
  }
  return result;
}

BOOST_AUTO_TEST_CASE(serialization_tests_bson) {
  removeContextFiles();
  blockmirror::chain::Context context;
  context.load();

  bsoncxx::builder::stream::document doc;
  blockmirror::serialization::BSONOArchive<bsoncxx::builder::stream::document>
      archive(context, doc);

  uint32_t uint32_test = 1314;
  BOOST_CHECK_EQUAL(toBSONStr("uint32", uint32_test), "{\"uint32\":1314}");
  float float_test = 1314.0f;
  BOOST_CHECK_EQUAL(toBSONStr("float", float_test), "{\"float\":1314.0}");
  blockmirror::Privkey key;
  key.fill(0);
  BOOST_CHECK_EQUAL(
      toBSONStr("key", key),
      "{\"key\":{\"$binary\":\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=\","
      "\"$type\":\"00\"}}");
  std::string strValue = "342532432";
  BOOST_CHECK_EQUAL(toBSONStr("strValue", strValue),
                    "{\"strValue\":\"342532432\"}");
  std::vector<std::string> strArray{"1111", "2222", "3333"};
  BOOST_CHECK_EQUAL(toBSONStr("strArray", strArray),
                    "{\"strArray\":[\"1111\",\"2222\",\"3333\"]}");

  auto var1 = VariantType2((int)-0x11);
  BOOST_CHECK_EQUAL(toBSONStr("var1", var1),
                    "{\"var1\":{\"type\":3,\"value\":-17}}");
  // auto var2 = VariantType((float)-0.11f);
  // BOOST_CHECK_EQUAL(toBSONStr("var2", var2), "{\"float\":1314.0}");
  // auto var3 = VariantType(std::string{"112233"});
  // BOOST_CHECK_EQUAL(toBSONStr("var3", var3), "{\"float\":1314.0}");
  // auto var4 = VariantType((unsigned int)0x11);
  // BOOST_CHECK_EQUAL(toBSONStr("var4", var4), "{\"float\":1314.0}");
  // auto var5 = VariantType(std::vector<uint8_t>{8, 6, 4, 2});
  // BOOST_CHECK_EQUAL(toBSONStr("var5", var5), "{\"float\":1314.0}");
  // auto var6 = VariantType(std::array<uint8_t, 4>{8, 6, 4, 2});
  // BOOST_CHECK_EQUAL(toBSONStr("var6", var6), "{\"float\":1314.0}");
  // auto var7 = VariantType(std::make_shared<int>(0x11));
  // BOOST_CHECK_EQUAL(toBSONStr("var7", var7), "{\"float\":1314.0}");

  blockmirror::chain::BlockHeader block1;
  // initFullBlock(block);

  archive << block1;

  std::cout << bsoncxx::to_json(doc.view()) << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()