#include<boost/test/unit_test.hpp>
#include<blockmirror/store/transaction_store.h>
#include <blockmirror/common.h>

BOOST_AUTO_TEST_SUITE(transaction_tests)

BOOST_AUTO_TEST_CASE(transaction_tests_ok) {

    blockmirror::store::TransactionStore ts1;
    boost::filesystem::path p("/ze");
    ts1.load(p);

    blockmirror::Pubkey pk1{1,2,3,4,5,6,7,8};
    blockmirror::Pubkey pk2{1,2,3,4,5,5};

    blockmirror::chain::TransactionPtr tPtr1 = 
    std::make_shared<blockmirror::chain::Transaction>(blockmirror::chain::scri::Transfer(pk1, 1000000));
    blockmirror::chain::TransactionPtr tPtr2 = 
    std::make_shared<blockmirror::chain::Transaction>(blockmirror::chain::scri::Transfer(pk2, 2000000));

    //ts1.add(tPtr1);
    //ts1.add(tPtr2,1);
    //blockmirror::Hash256 h1 = tPtr1->getHash();
    //blockmirror::Hash256 h2 = tPtr2->getHash();

/*     for(size_t i = 0; i < h1.size(); i++)
    {
        //std::cout<<h1[i]<<",";
    }
    std::cout<<std::endl; */

/*     for(size_t i = 0; i < h2.size(); i++)
    {
        std::cout<<h2[i]<<",";
    }
    std::cout<<std::endl; */

    
    //bool b1 = ts1.exist(h1);
    //bool b2 = ts1.exist(h2);

    //std::cout<<"b1 is:"<<b1<<std::endl;
    //std::cout<<"b2 is:"<<b2<<std::endl;

    ts1.remove(3);

    std::vector<blockmirror::chain::TransactionPtr> vv;
    vv = ts1.copy();

    std::cout<<"vv size:"<<vv.size()<<std::endl;
    for(size_t i = 0; i < vv.size(); i++)
    {
        blockmirror::Hash256 h = vv[i]->getHash();
        for(auto j:h)
        {
            std::cout<<j;
        }
        std::cout<<std::endl;

        std::cout<<"b is:"<<ts1.exist(h)<<std::endl;
        std::cout<<"expire:"<<vv[i]->getExpire()<<std::endl;

        //ts1.modify(h,blockmirror::store::TransactionStore::PACK_STATUS);
    }
    

    ts1.close(); 

    std::cout<<"transaction test end."<<std::endl;
}

BOOST_AUTO_TEST_SUITE_END()