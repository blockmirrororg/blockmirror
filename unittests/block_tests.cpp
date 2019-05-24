// 测试列表:
// 1. chain各种结构的JSON
// 2. chain各种结构的二进制
// 3. HASH计算是否正确
// 4. 签名和验证是否正确
#include <blockmirror/chain/block.h>
#include <blockmirror/serialization/binary_iarchive.h>
#include <blockmirror/serialization/binary_oarchive.h>
#include <blockmirror/serialization/json_oarchive.h>
#include <blockmirror/serialization/ptree_iarchive.h>
#include <boost/algorithm/hex.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/test/unit_test.hpp>

#include <fstream>
#include <iostream>

using Bin = blockmirror::serialization::BinaryOArchive<std::ostringstream>;
using JSON = blockmirror::serialization::JSONOArchive<std::ostringstream>;

std::string bp1Pub =
    "02ECCAE0C5766164670E17C7F6796294375BE8CD3F3F135C035CE8C3024D54B6D4";
std::string bp1Priv =
    "068972C2BB42DF301DA05BBCEF718A8516FA03F10DC62BA5A08223516B99F200";
std::string bp2Pub =
    "025D6860D335281760E2197A485A1BF7779396D4A1ACAAB6830480AA81CA17327B";
std::string bp2Priv =
    "9DC54FB3E7493E97D7B9130DAB4CC75275DE02199FD19E4A4CBDBEF539F6D496";
std::string bp3Pub =
    "03A81904E7BFE3C5F376B4DFB030EDC486E81F84A27CABD2AD492C4C2EB17344DB";
std::string bp3Priv =
    "B19375F1D6A3CC299C27DD6F793E91234B6E8CA9692131E6E8F320B83F84FF2C";

std::string APub =
    "0349622F329912F575DB5E1FC15849CA78DD0A2DD0EEBF34D2FD9683A2C2B3B924";
std::string APriv =
    "32CD6A9C3B4D58C06606513A7C630307C3E08A42599C54BDB17D5C81EC847B9E";
std::string BPub =
    "0213E21D6D3A4D64994E938F51A128861DEA7395A456C08F62A4549DF904D4B525";
std::string BPriv =
    "95D4B0DF5B1069B47F35C8C7A6764BB8B760D0359B6C1221DDCB46CE5830E14C";

blockmirror::Privkey K(const std::string &str) {
  blockmirror::Privkey priv;
  boost::algorithm::unhex(str, priv.begin());
  return priv;
}

blockmirror::Pubkey P(const std::string &str) {
  blockmirror::Pubkey pub;
  boost::algorithm::unhex(str, pub.begin());
  return pub;
}

template <typename Archive, typename T>
std::string SS(const T &obj) {
  std::ostringstream oss;
  Archive archive(oss);
  archive << obj;
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
  std::istringstream iss(value);
  blockmirror::serialization::BinaryIArchive<std::istringstream> archive(iss);
  archive >> out;
}

BOOST_AUTO_TEST_SUITE(block_tests)

/*
BOOST_AUTO_TEST_CASE(block_tests_transaction) {
  // std::string trxJSON =
  //     "{\"expire\":2,\"nonce\":0,\"script\":{\"type\":3,\"value\":{\"name\":"
  //     "\"stock\",\"desc\":\"代表五个浮点 股价上限 下限 最高 最低 平均 "
  //     "发送一个\",\"dataFormat\":\"0101010101\",\"validScript\":\"01\","
  //     "\"resultScript\":\"02\"}},\"signatures\":[{\"signer\":"
  //     "\"02ECCAE0C5766164670E17C7F6796294375BE8CD3F3F135C035CE8C3024D54B6D4\","
  //     "\"signature\":"
  //     "\"D4F32C8838FAC8BE5D83D54489250E98405549E4BBDCF538CD4477454486651E478EFA"
  //     "6E2EC236D5BDAC4FE91D4892B2C74816FFF57E128E55A47D84E9CD8249\"},{"
  //     "\"signer\":"
  //     "\"025D6860D335281760E2197A485A1BF7779396D4A1ACAAB6830480AA81CA17327B\","
  //     "\"signature\":"
  //     "\"ABEEE528CF22F12EB59751318E9AA4831127CB5EB1DEB672582E8089F6F90034E2B1A3"
  //     "236B6174BD8B9F46234E57F55F4592C6999B96DEF220E16DB0D7171F7F\"}]}";
  std::string trxJSON =
      "{\"expire\":1000000,\"nonce\":0,\"script\":{\"type\":4,\"value\":{"
      "\"format\":\"stock\",\"name\":\"aapl\",\"desc\":\"苹果的股票啊\"}},"
      "\"signatures\":[{\"signer\":"
      "\"0213E21D6D3A4D64994E938F51A128861DEA7395A456C08F62A4549DF904D4B525\","
      "\"signature\":"
      "\"B5789E9BCE973898E98534780E845CF744A70352C575AA18CD2E7D1B035B287D3D74E5"
      "6EEF3E194636C4117D30840EE57403863B00CB12B1EFA9850A7847D671\"}]}";
  blockmirror::chain::TransactionSigned trx;
  IJ(trxJSON, trx);
  std::cout << SS<JSON>(trx) << std::endl;
  BOOST_CHECK(trx.verify());
  auto bp1Priv =
      "95D4B0DF5B1069B47F35C8C7A6764BB8B760D0359B6C1221DDCB46CE5830E14C";
  auto bp2Priv =
      "9DC54FB3E7493E97D7B9130DAB4CC75275DE02199FD19E4A4CBDBEF539F6D496";

  trx.addSign(K(bp1Priv));
  trx.addSign(K(bp2Priv));
  std::cout << SS<JSON>(trx) << std::endl;
  B_LOG("HASH: {:spn}",
        spdlog::to_hex(SS<Bin>((blockmirror::chain::Transaction &)trx)));
  B_LOG("HASH: {:spn}", spdlog::to_hex(trx.getHash()));
  
  std::cout << SS<JSON>(blockmirror::chain::Transaction(blockmirror::chain::scri::NewData("aaa", "bbb", "ccc"))) << std::endl;
  
  B_LOG("{:spn}",
        spdlog::to_hex(SS<Bin>(blockmirror::chain::Transaction(blockmirror::chain::scri::NewData("aaa", "bbb", "ccc")))));
}
*/

BOOST_AUTO_TEST_CASE(block_tests_generating) {
  // 接收RPC或者P2P网络中的交易和数据
  // 在网络线程中序列化检查完签名
  // 投递到主线程中尝试执行交易 如果出错则丢弃
  // 否则交易和数据保存到未确认交易中
  // 接收到区块时会会标记交易状态

  // 出块流程模拟
  blockmirror::chain::Block block;
  // 设置父区块
  block.setGenesis();  // 创世块
  // block.setPrevious(parentBlock);
  // 填充coinbase的受益账号
  block.setCoinbase(P(APub));
  // 将未确认交易加入
  // 1. 转账 A => B
  blockmirror::chain::TransactionSignedPtr transfer =
      std::make_shared<blockmirror::chain::TransactionSigned>(
          blockmirror::chain::scri::Transfer(P(BPub), 1000000));
  transfer->setExpire(2);
  transfer->setNonce();
  transfer->addSign(K(APriv));
  BOOST_CHECK(transfer->verify());
  block.addTransaction(transfer);
  // 2. BP加入 bp1和bp2 支持 bp3加入
  blockmirror::chain::TransactionSignedPtr bpjoin =
      std::make_shared<blockmirror::chain::TransactionSigned>(
          blockmirror::chain::scri::BPJoin(P(bp3Pub)));
  bpjoin->setExpire(2);
  bpjoin->setNonce();
  bpjoin->addSign(K(bp1Priv));
  bpjoin->addSign(K(bp2Priv));
  BOOST_CHECK(bpjoin->verify());
  block.addTransaction(bpjoin);
  // 3. BP退出 bp1和bp2 投票bp3退出
  blockmirror::chain::TransactionSignedPtr bpexit =
      std::make_shared<blockmirror::chain::TransactionSigned>(
          blockmirror::chain::scri::BPExit(P(bp3Pub)));
  bpexit->setExpire(2);
  bpexit->setNonce();
  bpexit->addSign(K(bp1Priv));
  bpexit->addSign(K(bp2Priv));
  BOOST_CHECK(bpjoin->verify());
  block.addTransaction(bpexit);
  // 4. 新的数据格式 bp1和bp2 提案新的数据格式
  using blockmirror::chain::scri::NewFormat;
  blockmirror::chain::TransactionSignedPtr newformat =
      std::make_shared<blockmirror::chain::TransactionSigned>(NewFormat(
          "stock", "股票类型的描述",
          {NewFormat::TYPE_FLOAT, NewFormat::TYPE_FLOAT, NewFormat::TYPE_FLOAT,
           NewFormat::TYPE_FLOAT, NewFormat::TYPE_FLOAT},
          {0x01}, {0x02}));
  newformat->setExpire(2);
  newformat->setNonce();
  newformat->addSign(K(bp1Priv));
  newformat->addSign(K(bp2Priv));
  BOOST_CHECK(bpjoin->verify());
  block.addTransaction(newformat);
  // 5. 新的数据种类
  blockmirror::chain::TransactionSignedPtr newdata =
      std::make_shared<blockmirror::chain::TransactionSigned>(
          blockmirror::chain::scri::NewData("stock", "alibaba",
                                            "阿里巴巴在纳斯达克的股票数据"));
  newdata->setExpire(2);
  newdata->setNonce();
  newdata->addSign(K(bp1Priv));
  newdata->addSign(K(bp2Priv));
  BOOST_CHECK(bpjoin->verify());
  block.addTransaction(newdata);
  // BP1提供的数据
  blockmirror::chain::DataBPPtr bp1Data =
      std::make_shared<blockmirror::chain::DataBP>(P(bp1Pub));
  blockmirror::chain::DataSignedPtr bp1Apple =
      std::make_shared<blockmirror::chain::DataSigned>(
          "APPLE", std::vector<uint8_t>{0x11, 0x22, 0x33, 0x44});
  blockmirror::chain::DataSignedPtr bp1Google =
      std::make_shared<blockmirror::chain::DataSigned>(
          "GOOGLE", std::vector<uint8_t>{0x11, 0x22, 0x33, 0x44});
  bp1Apple->sign(K(bp1Priv), block.getHeight());
  bp1Google->sign(K(bp1Priv), block.getHeight());
  BOOST_CHECK(bp1Apple->verify(P(bp1Pub), block.getHeight()));
  BOOST_CHECK(bp1Google->verify(P(bp1Pub), block.getHeight()));
  bp1Data->addData(bp1Apple);
  bp1Data->addData(bp1Google);
  block.addDataBP(bp1Data);

  // BP2提供的数据
  blockmirror::chain::DataBPPtr bp2Data =
      std::make_shared<blockmirror::chain::DataBP>(P(bp2Pub));
  blockmirror::chain::DataSignedPtr bp2Apple =
      std::make_shared<blockmirror::chain::DataSigned>(
          "APPLE", std::vector<uint8_t>{0x11, 0x22, 0x33, 0x44});
  blockmirror::chain::DataSignedPtr bp2Google =
      std::make_shared<blockmirror::chain::DataSigned>(
          "GOOGLE", std::vector<uint8_t>{0x11, 0x22, 0x33, 0x44});
  bp2Apple->sign(K(bp2Priv), block.getHeight());
  bp2Google->sign(K(bp2Priv), block.getHeight());
  BOOST_CHECK(bp2Apple->verify(P(bp2Pub), block.getHeight()));
  BOOST_CHECK(bp2Google->verify(P(bp2Pub), block.getHeight()));
  bp2Data->addData(bp2Apple);
  bp2Data->addData(bp2Google);
  block.addDataBP(bp2Data);

  // 计算结果数据 计算默克数 签名
  block.finalize(K(bp1Priv));

  BOOST_CHECK(block.verify());
  BOOST_CHECK(block.verifyMerkle());

  // 序列化测试
  std::string strJSON = SS<JSON>(block);
  std::string strBin = SS<Bin>(block);

  // std::cout << strJSON << std::endl;

  blockmirror::chain::Block blockFromJSON;
  IJ(SS<JSON>(block), blockFromJSON);
  blockmirror::chain::Block blockFromBin;
  IS(SS<Bin>(block), blockFromBin);

  BOOST_CHECK_EQUAL(SS<JSON>(blockFromJSON), strJSON);
  BOOST_CHECK_EQUAL(SS<JSON>(blockFromBin), strJSON);
  BOOST_CHECK_EQUAL(SS<Bin>(blockFromJSON), strBin);
  BOOST_CHECK_EQUAL(SS<Bin>(blockFromBin), strBin);
}

BOOST_AUTO_TEST_SUITE_END()