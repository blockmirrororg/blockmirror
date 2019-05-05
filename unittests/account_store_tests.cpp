#include<boost/test/unit_test.hpp>
#include<blockmirror/store/account_store.h>

BOOST_AUTO_TEST_SUITE(account_tests)

BOOST_AUTO_TEST_CASE(account_tests_ok) {

    blockmirror::store::AccountStore as1;
    boost::filesystem::path p("/ze");
    as1.load(p);

    blockmirror::Pubkey pk1{1,2,3,4,5,6,7,8};
    blockmirror::Pubkey pk2{1,2,3,4,5,5};

    //blockmirror::store::AccountPtr p1 = 
    //std::make_shared<blockmirror::store::Account>(bpj1);
    //as1.add(pk1,200);
    //as1.transfer(pk2,pk1,20);

    uint64_t p2 = as1.query(pk1);
    std::cout<<p2<<std::endl;

    uint64_t p3 = as1.query(pk2);
    std::cout<<p3<<std::endl;
 

    as1.close(); 

    std::cout<<"account test end."<<std::endl;
}

BOOST_AUTO_TEST_SUITE_END()