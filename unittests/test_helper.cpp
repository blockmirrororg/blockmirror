#include "test_helper.h"

void initFullBlock(blockmirror::chain::Block &block) {
  float initializeData[] = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f};
  for (size_t i = 0; i < sizeof(initializeData) / sizeof(initializeData[0]);
       i++) {
    boost::endian::native_to_little_inplace(initializeData[i]);
  }
  std::vector<uint8_t> nativeData((uint8_t *)initializeData,
                                  (uint8_t *)(initializeData + 5));

  // 设置父区块
  block.setGenesis();  // 创世块
  block.setTimestamp(1559541065000);
  // block.setPrevious(parentBlock);
  // 填充coinbase的受益账号
  block.setCoinbase(PUB(pubZhaoliu));
  block.getCoinbase()->setNonce(778905127);
  // 将未确认交易加入
  // 1. 转账 A => B
  blockmirror::chain::TransactionSignedPtr transfer =
      std::make_shared<blockmirror::chain::TransactionSigned>(
          blockmirror::chain::scri::Transfer(PUB(pubSunqi), 1000000));
  transfer->setExpire(2);
  transfer->setNonce(2807220144);
  transfer->addSign(PRIV(privZhaoliu));
  // BOOST_CHECK(transfer->verify());
  block.addTransaction(transfer);
  // 2. BP加入 bp1和bp2 支持 bp3加入
  blockmirror::chain::TransactionSignedPtr bpjoin =
      std::make_shared<blockmirror::chain::TransactionSigned>(
          blockmirror::chain::scri::BPJoin(PUB(pubWangwu)));
  bpjoin->setExpire(2);
  bpjoin->setNonce(2206937323);
  bpjoin->addSign(PRIV(privZhangsan));
  bpjoin->addSign(PRIV(privLisi));
  // BOOST_CHECK(bpjoin->verify());
  block.addTransaction(bpjoin);
  // 3. BP退出 bp1和bp2 投票bp3退出
  blockmirror::chain::TransactionSignedPtr bpexit =
      std::make_shared<blockmirror::chain::TransactionSigned>(
          blockmirror::chain::scri::BPExit(PUB(pubWangwu)));
  bpexit->setExpire(2);
  bpexit->setNonce(224074457);
  bpexit->addSign(PRIV(privZhangsan));
  bpexit->addSign(PRIV(privLisi));
  // BOOST_CHECK(bpjoin->verify());
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
  newformat->setNonce(4074770719);
  newformat->addSign(PRIV(privZhangsan));
  newformat->addSign(PRIV(privLisi));
  // BOOST_CHECK(bpjoin->verify());
  block.addTransaction(newformat);
  // 5. 新的数据种类
  blockmirror::chain::TransactionSignedPtr newdata =
      std::make_shared<blockmirror::chain::TransactionSigned>(
          blockmirror::chain::scri::NewData("stock", "APPLE", "苹果股票"));
  newdata->setExpire(2);
  newdata->setNonce(2224478161);
  newdata->addSign(PRIV(privZhangsan));
  newdata->addSign(PRIV(privLisi));
  // BOOST_CHECK(bpjoin->verify());
  block.addTransaction(newdata);
  blockmirror::chain::TransactionSignedPtr newdata2 =
      std::make_shared<blockmirror::chain::TransactionSigned>(
          blockmirror::chain::scri::NewData("stock", "GOOGLE", "谷歌股票"));
  newdata2->setExpire(2);
  newdata2->setNonce(592043445);
  newdata2->addSign(PRIV(privZhangsan));
  newdata2->addSign(PRIV(privLisi));
  // BOOST_CHECK(bpjoin->verify());
  block.addTransaction(newdata2);

  // BP1提供的数据
  blockmirror::chain::DataBPPtr bp1Data =
      std::make_shared<blockmirror::chain::DataBP>(PUB(pubZhangsan));
  blockmirror::chain::DataSignedPtr bp1Apple =
      std::make_shared<blockmirror::chain::DataSigned>("APPLE", nativeData);
  blockmirror::chain::DataSignedPtr bp1Google =
      std::make_shared<blockmirror::chain::DataSigned>("GOOGLE", nativeData);
  bp1Apple->sign(PRIV(privZhangsan), block.getHeight());
  bp1Google->sign(PRIV(privZhangsan), block.getHeight());
  // BOOST_CHECK(bp1Apple->verify(PUB(pubZhangsan), block.getHeight()));
  // BOOST_CHECK(bp1Google->verify(PUB(pubZhangsan), block.getHeight()));
  bp1Data->addData(bp1Apple);
  bp1Data->addData(bp1Google);
  block.addDataBP(bp1Data);

  // BP2提供的数据
  blockmirror::chain::DataBPPtr bp2Data =
      std::make_shared<blockmirror::chain::DataBP>(PUB(pubLisi));
  blockmirror::chain::DataSignedPtr bp2Apple =
      std::make_shared<blockmirror::chain::DataSigned>("APPLE", nativeData);
  blockmirror::chain::DataSignedPtr bp2Google =
      std::make_shared<blockmirror::chain::DataSigned>("GOOGLE", nativeData);
  bp2Apple->sign(PRIV(privLisi), block.getHeight());
  bp2Google->sign(PRIV(privLisi), block.getHeight());
  // BOOST_CHECK(bp2Apple->verify(PUB(pubLisi), block.getHeight()));
  // BOOST_CHECK(bp2Google->verify(PUB(pubLisi), block.getHeight()));
  bp2Data->addData(bp2Apple);
  bp2Data->addData(bp2Google);
  block.addDataBP(bp2Data);

  // 计算结果数据 计算默克数 签名
  block.finalize(PRIV(privZhangsan));
}

void removeContextFiles() {
  for (uint32_t i = 0; i < 100; i++) {
    std::string file =
        std::string("./") + boost::lexical_cast<std::string>(i) + ".block";
    if (!boost::filesystem::remove(file)) {
      break;
    }
  }
  boost::filesystem::remove("./index");
  boost::filesystem::remove("./account");
  boost::filesystem::remove("./bps");
  boost::filesystem::remove("./data");
  boost::filesystem::remove("./format");
  boost::filesystem::remove("./transaction");
  boost::filesystem::remove("./head");
}

blockmirror::Privkey PRIV(const std::string &str) {
  blockmirror::Privkey key;
  blockmirror::fromHex(key, str);
  return key;
}
blockmirror::Pubkey PUB(const std::string &str) {
  blockmirror::Pubkey key;
  blockmirror::fromHex(key, str);
  return key;
}

std::string pubZhangsan =
    "02ECCAE0C5766164670E17C7F6796294375BE8CD3F3F135C035CE8C3024D54B6D4";
std::string privZhangsan =
    "068972C2BB42DF301DA05BBCEF718A8516FA03F10DC62BA5A08223516B99F200";
std::string pubLisi =
    "025D6860D335281760E2197A485A1BF7779396D4A1ACAAB6830480AA81CA17327B";
std::string privLisi =
    "9DC54FB3E7493E97D7B9130DAB4CC75275DE02199FD19E4A4CBDBEF539F6D496";
std::string pubWangwu =
    "03A81904E7BFE3C5F376B4DFB030EDC486E81F84A27CABD2AD492C4C2EB17344DB";
std::string privWangwu =
    "B19375F1D6A3CC299C27DD6F793E91234B6E8CA9692131E6E8F320B83F84FF2C";
std::string pubZhaoliu =
    "0349622F329912F575DB5E1FC15849CA78DD0A2DD0EEBF34D2FD9683A2C2B3B924";
std::string privZhaoliu =
    "32CD6A9C3B4D58C06606513A7C630307C3E08A42599C54BDB17D5C81EC847B9E";
std::string pubSunqi =
    "0213E21D6D3A4D64994E938F51A128861DEA7395A456C08F62A4549DF904D4B525";
std::string privSunqi =
    "95D4B0DF5B1069B47F35C8C7A6764BB8B760D0359B6C1221DDCB46CE5830E14C";