#include<boost/test/unit_test.hpp>
#include<blockmirror/store/bps_store.h>

BOOST_AUTO_TEST_SUITE(bps_tests)

BOOST_AUTO_TEST_CASE(bps_tests_ok) {

    blockmirror::store::BpsStore bs1;
    boost::filesystem::path p("/ze");
    bs1.load(p);

    blockmirror::Pubkey pk1{1,2,3,4,5,6,7,8};
    blockmirror::Pubkey pk2{1,2,3,4,5,5};
    blockmirror::chain::scri::BPJoin bpj1
    {pk1};
    blockmirror::store::BPJoinPtr p1 = 
    std::make_shared<blockmirror::chain::scri::BPJoin>(bpj1);
    bs1.add(p1);

    blockmirror::store::BPJoinPtr p2 = bs1.query(pk1);
    if (p2 == nullptr) {
        std::cout<<"nullptr"<<std::endl;
    }
    else{
        blockmirror::Pubkey pk = p2->getBP();
        for(auto i:pk)
        std::cout<<(int)i<<"";

        std::cout<<std::endl;
    }

    blockmirror::store::BPJoinPtr p3 = bs1.query(pk2);
    if (p3 == nullptr) {
        std::cout<<"nullptr"<<std::endl;
    }
    else{
        blockmirror::Pubkey pk = p3->getBP();
        for(auto i:pk)
        std::cout<<(int)i<<"";

        std::cout<<std::endl;
    }

    bs1.close(); 

    std::cout<<"bps test end."<<std::endl;
}

BOOST_AUTO_TEST_SUITE_END()