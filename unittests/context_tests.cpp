#include<boost/test/unit_test.hpp>
#include<blockmirror/chain/context.h>
#include <blockmirror/common.h>
#include <boost/algorithm/hex.hpp>
#include<blockmirror/chain/context.h>
#include <blockmirror/store/account_store.h>
#include<blockmirror/store/bps_store.h>
#include<blockmirror/store/data_store.h>
#include<blockmirror/store/format_store.h>
#include<blockmirror/store/transaction_store.h>

BOOST_AUTO_TEST_SUITE(context_tests)

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

std::string APub =
    "0349622F329912F575DB5E1FC15849CA78DD0A2DD0EEBF34D2FD9683A2C2B3B924";
std::string APriv =
    "32CD6A9C3B4D58C06606513A7C630307C3E08A42599C54BDB17D5C81EC847B9E";
std::string BPub =
    "0213E21D6D3A4D64994E938F51A128861DEA7395A456C08F62A4549DF904D4B525";
std::string BPriv =
    "95D4B0DF5B1069B47F35C8C7A6764BB8B760D0359B6C1221DDCB46CE5830E14C";

BOOST_AUTO_TEST_CASE(context_tests_ok) {
    
    boost::filesystem::remove("./account");
    boost::filesystem::remove("./bps");
    boost::filesystem::remove("./data");
    boost::filesystem::remove("./format");
    boost::filesystem::remove("./transaction");

    blockmirror::chain::BlockPtr blockPtr = 
    std::make_shared<blockmirror::chain::Block>();

    //blockPtr->setCoinbase(P(APub),1000);
    {
        blockmirror::store::AccountStore store;
        store.load(".");
        store.add(P(APub),1000);
    }  
    
    blockmirror::chain::TransactionSignedPtr tPtr =
    std::make_shared<blockmirror::chain::TransactionSigned>(
        blockmirror::chain::scri::Transfer(P(BPub), 100));
    tPtr->addSign(K(APriv));
    blockPtr->addTransaction(tPtr);

    blockmirror::chain::TransactionSignedPtr tPtr1 =
    std::make_shared<blockmirror::chain::TransactionSigned>(
        blockmirror::chain::scri::BPJoin(P(APub)));
    tPtr1->addSign(K(APriv));
    blockPtr->addTransaction(tPtr1);

    blockmirror::chain::TransactionSignedPtr tPtr2 =
    std::make_shared<blockmirror::chain::TransactionSigned>(
        blockmirror::chain::scri::BPJoin(P(BPub)));
    tPtr2->addSign(K(BPriv));
    blockPtr->addTransaction(tPtr2);

    blockmirror::chain::TransactionSignedPtr tPtr3 =
    std::make_shared<blockmirror::chain::TransactionSigned>(
        blockmirror::chain::scri::BPExit(P(BPub)));
    tPtr3->addSign(K(BPriv));
    blockPtr->addTransaction(tPtr3);

    blockmirror::chain::TransactionSignedPtr tPtr4 =
    std::make_shared<blockmirror::chain::TransactionSigned>(
        blockmirror::chain::scri::NewData("aaa","111","rrt"));
    tPtr4->addSign(K(BPriv));
    blockPtr->addTransaction(tPtr4);

    blockmirror::chain::TransactionSignedPtr tPtr5 =
    std::make_shared<blockmirror::chain::TransactionSigned>(
        blockmirror::chain::scri::NewFormat("aaa","ggg",{1,3,5,7,9},{2,4,6,8},{6,6,6}));
    tPtr5->addSign(K(APriv));
    blockPtr->addTransaction(tPtr5);
    
    { 
        blockmirror::chain::Context c;
        c.load();
        c.apply(blockPtr);
        c.close();
 
        blockmirror::store::AccountStore accountStore;
        accountStore.load(".");
        BOOST_CHECK_EQUAL(accountStore.query(P(APub)), 900);
        BOOST_CHECK_EQUAL(accountStore.query(P(BPub)), 100);

        blockmirror::store::BpsStore bpsStore;
        bpsStore.load(".");
        BOOST_CHECK_EQUAL(bpsStore.contains(P(APub)),1);
        BOOST_CHECK_EQUAL(bpsStore.contains(P(BPub)),0);

        blockmirror::store::DataStore dataStore;
        dataStore.load(".");
        BOOST_CHECK(dataStore.query("111"));

        blockmirror::store::FormatStore formatStore;
        formatStore.load(".");
        BOOST_CHECK(formatStore.query("aaa"));

        blockmirror::store::TransactionStore transactionStore;
        transactionStore.load(".");
        BOOST_CHECK_EQUAL(transactionStore.contains(tPtr->getHashPtr()),1);
        BOOST_CHECK_EQUAL(transactionStore.contains(tPtr1->getHashPtr()),1);
        BOOST_CHECK_EQUAL(transactionStore.contains(tPtr2->getHashPtr()),1);
        BOOST_CHECK_EQUAL(transactionStore.contains(tPtr3->getHashPtr()),1);
        BOOST_CHECK_EQUAL(transactionStore.contains(tPtr4->getHashPtr()),1);
        BOOST_CHECK_EQUAL(transactionStore.contains(tPtr5->getHashPtr()),1);
    }
    {
        blockmirror::chain::Context c;
        c.load();
        c.rollback(blockPtr);
        c.close();
        
        blockmirror::store::AccountStore store;
        store.load(".");
        BOOST_CHECK_EQUAL(store.query(P(APub)), 1000);
        BOOST_CHECK_EQUAL(store.query(P(BPub)), 0);

        blockmirror::store::BpsStore bpsStore;
        bpsStore.load(".");
        BOOST_CHECK_EQUAL(bpsStore.contains(P(APub)),0);
        BOOST_CHECK_EQUAL(bpsStore.contains(P(BPub)),0);

        blockmirror::store::DataStore dataStore;
        dataStore.load(".");
        BOOST_CHECK(!dataStore.query("111"));

        blockmirror::store::FormatStore formatStore;
        formatStore.load(".");
        BOOST_CHECK(!formatStore.query("aaa"));

        blockmirror::store::TransactionStore transactionStore;
        transactionStore.load(".");
        BOOST_CHECK_EQUAL(transactionStore.contains(tPtr->getHashPtr()),0);
        BOOST_CHECK_EQUAL(transactionStore.contains(tPtr1->getHashPtr()),0);
        BOOST_CHECK_EQUAL(transactionStore.contains(tPtr2->getHashPtr()),0);
        BOOST_CHECK_EQUAL(transactionStore.contains(tPtr3->getHashPtr()),0);
        BOOST_CHECK_EQUAL(transactionStore.contains(tPtr4->getHashPtr()),0);
        BOOST_CHECK_EQUAL(transactionStore.contains(tPtr5->getHashPtr()),0);
    }
    {
        blockmirror::store::AccountStore store;
        store.load(".");
        BOOST_CHECK_EQUAL(store.query(P(APub)), 1000);
        BOOST_CHECK_EQUAL(store.query(P(BPub)), 0);

        blockmirror::store::BpsStore bpsStore;
        bpsStore.load(".");
        BOOST_CHECK_EQUAL(bpsStore.contains(P(APub)),0);
        BOOST_CHECK_EQUAL(bpsStore.contains(P(BPub)),0);

        blockmirror::store::DataStore dataStore;
        dataStore.load(".");
        BOOST_CHECK(!dataStore.query("111"));

        blockmirror::store::FormatStore formatStore;
        formatStore.load(".");
        BOOST_CHECK(!formatStore.query("aaa"));

        blockmirror::store::TransactionStore transactionStore;
        transactionStore.load(".");
        BOOST_CHECK_EQUAL(transactionStore.contains(tPtr->getHashPtr()),0);
        BOOST_CHECK_EQUAL(transactionStore.contains(tPtr1->getHashPtr()),0);
        BOOST_CHECK_EQUAL(transactionStore.contains(tPtr2->getHashPtr()),0);
        BOOST_CHECK_EQUAL(transactionStore.contains(tPtr3->getHashPtr()),0);
        BOOST_CHECK_EQUAL(transactionStore.contains(tPtr4->getHashPtr()),0);
        BOOST_CHECK_EQUAL(transactionStore.contains(tPtr5->getHashPtr()),0);
    }
}

BOOST_AUTO_TEST_SUITE_END()