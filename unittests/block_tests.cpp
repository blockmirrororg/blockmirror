// 测试列表:
// 1. chain各种结构的JSON
// 2. chain各种结构的二进制
// 3. HASH计算是否正确
// 4. 签名和验证是否正确
#include <blockmirror/chain/block.h>
#include <boost/test/unit_test.hpp>
#include "test_helper.h"

BOOST_AUTO_TEST_SUITE(block_tests)

BOOST_AUTO_TEST_CASE(block_tests_generating) {
  // 接收RPC或者P2P网络中的交易和数据
  // 在网络线程中序列化检查完签名
  // 投递到主线程中尝试执行交易 如果出错则丢弃
  // 否则交易和数据保存到未确认交易中
  // 接收到区块时会会标记交易状态

  // 出块流程模拟
  blockmirror::chain::Block block;

  initFullBlock(block);

  for (auto &trx : block.getTransactions()) {
    BOOST_CHECK(trx->verify());
  }

  BOOST_CHECK(block.verify());
  BOOST_CHECK(block.verifyMerkle());

  // 序列化测试
  std::string strJSON = SerOToJson(block);
  std::string strBin = SerOToBin(block);

  // std::cout << strJSON << std::endl;

  blockmirror::chain::Block blockFromJSON;
  SerOFromJSON(SerOToJson(block), blockFromJSON);
  blockmirror::chain::Block blockFromBin;
  SerOFromBin(SerOToBin(block), blockFromBin);

  BOOST_CHECK_EQUAL(SerOToJson(blockFromJSON), strJSON);
  BOOST_CHECK_EQUAL(SerOToJson(blockFromBin), strJSON);
  BOOST_CHECK_EQUAL(SerOToBin(blockFromJSON), strBin);
  BOOST_CHECK_EQUAL(SerOToBin(blockFromBin), strBin);
}

BOOST_AUTO_TEST_SUITE_END()