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

class TTTClass {
 private:
  friend class blockmirror::serialization::access;
  template <typename Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(val);
  }

 public:
  int val;
  TTTClass() : val(111) {}
};

using VariantType =
    boost::variant<unsigned int, float, std::string, int, std::vector<uint8_t>,
                   std::array<uint8_t, 4>, std::shared_ptr<int>>;
using VariantType2 = boost::variant<unsigned int, TTTClass>;

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
  removeContextFiles();
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

  uint32_t uint32_test = 1314;
  BOOST_CHECK_EQUAL(toBSONStr("uint32", uint32_test), "{\"uint32\":1314}");
  float float_test = 1314.0f;
  BOOST_CHECK_EQUAL(toBSONStr("float", float_test), "{\"float\":1314.0}");
  blockmirror::Privkey key;
  key.fill(0);
  BOOST_CHECK_EQUAL(
      toBSONStr("key", key),
      "{\"key\":"
      "\"0000000000000000000000000000000000000000000000000000000000000000\"}");
  std::string strValue = "342532432";
  BOOST_CHECK_EQUAL(toBSONStr("strValue", strValue),
                    "{\"strValue\":\"342532432\"}");
  std::vector<std::string> strArray{"1111", "2222", "3333"};
  BOOST_CHECK_EQUAL(toBSONStr("strArray", strArray),
                    "{\"strArray\":[\"1111\",\"2222\",\"3333\"]}");

  auto var1 = VariantType2((unsigned int)0x11);
  BOOST_CHECK_EQUAL(toBSONStr("var1", var1),
                    "{\"var1\":{\"type\":0,\"value\":17}}");
  auto var2 = VariantType2(TTTClass{});
  BOOST_CHECK_EQUAL(toBSONStr("var2", var2),
                    "{\"var2\":{\"type\":1,\"value\":{\"val\":111}}}");

  {
    removeContextFiles();
    blockmirror::chain::Context context;
    context.load();
    bsoncxx::builder::stream::document doc;
    blockmirror::serialization::BSONOArchive<bsoncxx::builder::stream::document>
        archive(context, doc);

    blockmirror::chain::BlockPtr blk =
        std::make_shared<blockmirror::chain::Block>();

    initFullBlock(*blk);

    BOOST_CHECK(context.getBpsStore().add(PUB(pubZhangsan)));
    BOOST_CHECK(context.getBpsStore().add(PUB(pubLisi)));

    BOOST_CHECK(context.apply(blk));

    archive << blk;

    auto json = bsoncxx::to_json(doc.view());
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
    std::string strResult =
        "{\"hash\":"
        "\"1D4499CF597CC9B20CBCDD3DC251D07B5C2C6E15DA876F364966FCFD3AF52B27\","
        "\"timestamp\":1559541065000,\"height\":1,\"previous\":"
        "\"0000000000000000000000000000000000000000000000000000000000000000\","
        "\"merkle\":"
        "\"2028FA1053DEAF74C18FFC1002735729952D1D8B7765DE1DB07021E8D0DCCA29\","
        "\"producer\":"
        "\"02ECCAE0C5766164670E17C7F6796294375BE8CD3F3F135C035CE8C3024D54B6D4\""
        ",\"coinbase\":{\"expire\":0,\"nonce\":778905127,\"script\":{\"type\":"
        "0,\"value\":{\"target\":"
        "\"0349622F329912F575DB5E1FC15849CA78DD0A2DD0EEBF34D2FD9683A2C2B3B924\""
        ",\"amount\":100000000}}},\"signature\":"
        "\"ECB68C544B8C9125358588B63F120191D40E101683D04882D5DC5BD2BDB7131E5F0B"
        "F2629096B1E8C5EBEFE725A68BBD9A1DD2585374EE7516427E054E071716\","
        "\"transactions\":[{\"expire\":2,\"nonce\":2807220144,\"script\":{"
        "\"type\":0,\"value\":{\"target\":"
        "\"0213E21D6D3A4D64994E938F51A128861DEA7395A456C08F62A4549DF904D4B525\""
        ",\"amount\":1000000}},\"signatures\":[{\"signer\":"
        "\"0349622F329912F575DB5E1FC15849CA78DD0A2DD0EEBF34D2FD9683A2C2B3B924\""
        ",\"signature\":"
        "\"ACA03289A5704157061FCF3701817D76683E4C22C6A61227B1E8C77EC70AFF39CDA5"
        "830B32AA9C4F22CB723835BC8E16D548DA60FB174D2AB7D767DEA246040D\"}]},{"
        "\"expire\":2,\"nonce\":2206937323,\"script\":{\"type\":1,\"value\":{"
        "\"bp\":"
        "\"03A81904E7BFE3C5F376B4DFB030EDC486E81F84A27CABD2AD492C4C2EB17344DB\""
        "}},\"signatures\":[{\"signer\":"
        "\"02ECCAE0C5766164670E17C7F6796294375BE8CD3F3F135C035CE8C3024D54B6D4\""
        ",\"signature\":"
        "\"18B59CCCA187235913B97198734D0A587A62002983E8E138EE21AF6AE2B2211447DA"
        "25611F75F180745F30024D9C2ED2877363F1355B44D7E2859EBF4F5FC543\"},{"
        "\"signer\":"
        "\"025D6860D335281760E2197A485A1BF7779396D4A1ACAAB6830480AA81CA17327B\""
        ",\"signature\":"
        "\"5F399D3DCC98AF3CA09003FF139B3D23182425A2A125663EE7D11FC8B0AC5F7791A2"
        "08BD43A213A55892DB5333734CC91ED3D40B8BB5C0B38A8007804F22D658\"}]},{"
        "\"expire\":2,\"nonce\":224074457,\"script\":{\"type\":2,\"value\":{"
        "\"bp\":"
        "\"03A81904E7BFE3C5F376B4DFB030EDC486E81F84A27CABD2AD492C4C2EB17344DB\""
        "}},\"signatures\":[{\"signer\":"
        "\"02ECCAE0C5766164670E17C7F6796294375BE8CD3F3F135C035CE8C3024D54B6D4\""
        ",\"signature\":"
        "\"B8138AF3B7EB7476D477F8A2267655AEF3D0ACF91BA8972CE5B7CF859D62F0032A61"
        "0B393A6974D240607CF813A5F9CF0AEB84EB484BC91F6CE9EE9B7499AE3C\"},{"
        "\"signer\":"
        "\"025D6860D335281760E2197A485A1BF7779396D4A1ACAAB6830480AA81CA17327B\""
        ",\"signature\":"
        "\"F3FEFB1D4F0020AF26BEBEF1C7F44771D551AECBEA5E8CC2A83E0179344871425359"
        "D065EE51934CAA53BB381BAE8C812FBC006491105CC2B6E2D243D6A9FA1D\"}]},{"
        "\"expire\":2,\"nonce\":4074770719,\"script\":{\"type\":3,\"value\":{"
        "\"name\":\"stock\",\"desc\":\"股票类型的描述\",\"dataFormat\":"
        "\"0101010101\",\"validScript\":\"01\",\"resultScript\":\"02\"}},"
        "\"signatures\":[{\"signer\":"
        "\"02ECCAE0C5766164670E17C7F6796294375BE8CD3F3F135C035CE8C3024D54B6D4\""
        ",\"signature\":"
        "\"AD91626483C3A298F3C5DE80CB88223EA0411BF39D23DF8A3FE3A6BD87B2DF00606B"
        "203BDF05BEE6EAF147B4997FBF289F0B3D42A50C152536D6AFD999B7ED4D\"},{"
        "\"signer\":"
        "\"025D6860D335281760E2197A485A1BF7779396D4A1ACAAB6830480AA81CA17327B\""
        ",\"signature\":"
        "\"ADA3BC3166691A5070F6F0C0953F87FD1CD308FCD653C757B3B6709F0374F8537D1C"
        "9F934B8DD52DC9287B47A7487798521277B67E297939014077C1A54B8B67\"}]},{"
        "\"expire\":2,\"nonce\":2224478161,\"script\":{\"type\":4,\"value\":{"
        "\"format\":\"stock\",\"name\":\"APPLE\",\"desc\":\"苹果股票\"}},"
        "\"signatures\":[{\"signer\":"
        "\"02ECCAE0C5766164670E17C7F6796294375BE8CD3F3F135C035CE8C3024D54B6D4\""
        ",\"signature\":"
        "\"136F20645F9228F91DBFD5F01BAD6286EB7EE44FAB207AFE1BA6CD2196CE595D5EF1"
        "6DA6BA82AD5B7EA1A4A69BE34C6E507FFDA127A31F0F930832FF573AB418\"},{"
        "\"signer\":"
        "\"025D6860D335281760E2197A485A1BF7779396D4A1ACAAB6830480AA81CA17327B\""
        ",\"signature\":"
        "\"F23670FF4FF2E627FFF0AE3475C075E8AD92B3E26B2D39E1FA8ECAD3F0167F739104"
        "D66AA5B59BD4051270FC7CF4F22FF36824DFC46E7072518DD55FE47A654A\"}]},{"
        "\"expire\":2,\"nonce\":592043445,\"script\":{\"type\":4,\"value\":{"
        "\"format\":\"stock\",\"name\":\"GOOGLE\",\"desc\":\"谷歌股票\"}},"
        "\"signatures\":[{\"signer\":"
        "\"02ECCAE0C5766164670E17C7F6796294375BE8CD3F3F135C035CE8C3024D54B6D4\""
        ",\"signature\":"
        "\"99EC9F3F6407E5DBD59EAAAE069B08BF4EE0F69E045535E8DBBF93408BF862611D28"
        "0C0C82071DA8411EC8B22D5ECB393A7721C71F79F040799C6FB69C7D046A\"},{"
        "\"signer\":"
        "\"025D6860D335281760E2197A485A1BF7779396D4A1ACAAB6830480AA81CA17327B\""
        ",\"signature\":"
        "\"CAF95932B574DF522D1C7456098EE7DFE71A9DB486D03F46874E3FC81661D81368BE"
        "810AE085F6926AA5F6C69DF19C095E376AC38EB5A0D71BD799D4B42E841B\"}]}],"
        "\"datas\":[{\"bp\":"
        "\"02ECCAE0C5766164670E17C7F6796294375BE8CD3F3F135C035CE8C3024D54B6D4\""
        ",\"datas\":[{\"name\":\"APPLE\",\"data\":\"0.1,0.2,0.3,0.4,0.5\","
        "\"signature\":"
        "\"38FA83348143F89904F262C93EB2D453D0696E9708158CD28037423668652F4330A2"
        "F7B47B61A23FB121FFAC00956FBB83AFE81773628CFB46E05F7F12882C31\"},{"
        "\"name\":\"GOOGLE\",\"data\":\"716.532,0,0.3,0.4,0.5\",\"signature\":"
        "\"EBC249DFAE609DA5A1ECC4B83A87D14040B842644FCD6169D68E1B6DBA4D6923C0D8"
        "B5905735B0965FA9E479C3E22FD795404A8B88E62F8FA4A92BF06B132123\"}]},{"
        "\"bp\":"
        "\"025D6860D335281760E2197A485A1BF7779396D4A1ACAAB6830480AA81CA17327B\""
        ",\"datas\":[{\"name\":\"APPLE\",\"data\":\"0.1,0.2,0.3,0.4,0.5\","
        "\"signature\":"
        "\"7E939C76673B76BAD7B4A9DD3770B5148CA8A112D2B8EE2863AC4BC0138E5B380DF1"
        "14DF7BDBD5EF502E48D0A3AC0F2205619E39EC10FB4F8DE4D90A573D2C6C\"},{"
        "\"name\":\"GOOGLE\",\"data\":\"0.1,0.2,0.3,0.4,0.5\",\"signature\":"
        "\"971439A9E01B15B105DE50E3273DA94197502C3D1F0C44F98EABCE1A191F2F0DB8ED"
        "5DFC1FCF4479BBE862868AF6415A5AA22FAB861CCF47D5E50B808A949310\"}]}]}";
    std::cout << result << std::endl;
    std::cout << strResult << std::endl;

    if (result.length() != strResult.length()) {
      BOOST_CHECK_MESSAGE(false, "BAD Length");
    } else {
      for (size_t i = 0; i < result.length(); i++) {
        if (result[i] != strResult[i]) {
          BOOST_CHECK_MESSAGE(false, &result[i]);
          break;
        }
      }
    }

    BOOST_CHECK_EQUAL(result, strResult);
  }
}

BOOST_AUTO_TEST_SUITE_END()