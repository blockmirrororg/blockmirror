#include <blockmirror/store/block_store.h>
#include <boost/algorithm/hex.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(blockstore_tests)

BOOST_AUTO_TEST_CASE(blockstore_tests_simple) {
  blockmirror::store::BlockStore store;

  blockmirror::chain::BlockPtr block1 =
      std::make_shared<blockmirror::chain::Block>();
  blockmirror::chain::BlockPtr block2 =
      std::make_shared<blockmirror::chain::Block>();
  block2->setPrevious(*block1);
  block1->getHash();
  block2->getHash();

  store.load(".");

  BOOST_CHECK_EQUAL(nullptr, store.getBlock(block1->getHash()).get());
  BOOST_CHECK_EQUAL(nullptr, store.getBlock(block2->getHash()).get());

  BOOST_CHECK(store.addBlock(block1));
  BOOST_CHECK(store.addBlock(block2));
  BOOST_CHECK(!store.addBlock(block2));

  BOOST_CHECK(store.contains(block1->getHashPtr()));
  BOOST_CHECK(store.contains(block1->getHash()));
  BOOST_CHECK(store.contains(block2->getHashPtr()));
  BOOST_CHECK(store.contains(block2->getHash()));

  BOOST_CHECK_EQUAL(block1.get(), store.getBlock(block1->getHash()).get());
  BOOST_CHECK_EQUAL(block2.get(), store.getBlock(block2->getHash()).get());

  BOOST_CHECK_EQUAL(block1.use_count(), 3);
  store.flushBlock(1);  // 此时block1已经入了硬盘
  BOOST_CHECK_EQUAL(block1.use_count(), 1);

  auto blk1 = store.getBlock(block1->getHashPtr());
  BOOST_CHECK_EQUAL_COLLECTIONS(blk1->getHash().begin(), blk1->getHash().end(),
                                block1->getHash().begin(),
                                block1->getHash().end());
  BOOST_CHECK_NE(blk1.get(), block1.get());
}

BOOST_AUTO_TEST_CASE(blockstore_tests_branch) {
  blockmirror::store::BlockStore store;
  std::vector<blockmirror::chain::BlockPtr> blks;
  for (size_t i = 0; i < 10; i++) {
    blks.push_back(std::make_shared<blockmirror::chain::Block>());
    blks.back()->setCoinbase({0});
    blks.back()->finalize({0x12, 0xb0, 0x04, 0xff, 0xf7, 0xf4, 0xb6, 0x9e,
                           0xf8, 0x65, 0x0e, 0x76, 0x7f, 0x18, 0xf1, 0x1e,
                           0xde, 0x15, 0x81, 0x48, 0xb4, 0x25, 0x66, 0x07,
                           0x23, 0xb9, 0xf9, 0xa6, 0x6e, 0x61, 0xf7, 0x47});
  }

  // 0 -> 1 -> 3 -> 5 -> 7 -> 9
  // 0 -> 2 -> 4 -> 6 -> 8
  blks[0]->setGenesis();
  blks[1]->setPrevious(*blks[0]);
  for (size_t i = 2; i <= 9; i++) {
    blks[i]->setPrevious(*blks[i - 2]);
  }

  for (auto &blk : blks) {
    store.addBlock(blk);
  }

  auto head = blks[8];
  auto fork = blks[9];
  std::vector<blockmirror::chain::BlockPtr> back;
  std::vector<blockmirror::chain::BlockPtr> forward;
  // 长度不够回撤的
  BOOST_CHECK(!store.shouldSwitch(fork, head, back, forward));
  // 回滚细节 back 8 6 4 2 forward 9 7 5 3 1
  BOOST_CHECK(store.shouldSwitch(head, fork, back, forward));
  BOOST_CHECK(back[0]->getHash() == blks[8]->getHash());
  BOOST_CHECK(back[1]->getHash() == blks[6]->getHash());
  BOOST_CHECK(back[2]->getHash() == blks[4]->getHash());
  BOOST_CHECK(back[3]->getHash() == blks[2]->getHash());
  BOOST_CHECK(forward[0]->getHash() == blks[9]->getHash());
  BOOST_CHECK(forward[1]->getHash() == blks[7]->getHash());
  BOOST_CHECK(forward[2]->getHash() == blks[5]->getHash());
  BOOST_CHECK(forward[3]->getHash() == blks[3]->getHash());
  BOOST_CHECK(forward[4]->getHash() == blks[1]->getHash());
}

BOOST_AUTO_TEST_SUITE_END()
