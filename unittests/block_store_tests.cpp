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

BOOST_AUTO_TEST_SUITE_END()
