#include<boost/test/unit_test.hpp>
#include<blockmirror/store/data_store.h>

BOOST_AUTO_TEST_SUITE(data_tests)

BOOST_AUTO_TEST_CASE(data_tests_ok) {

    blockmirror::store::DataStore dt1;
    boost::filesystem::path p("/ze");
    dt1.load(p);

    
    blockmirror::chain::scri::NewData n1
    {std::string{"111"},std::string{"222"},std::string{"333"}};
    blockmirror::store::NewDataPtr p1 = 
    std::make_shared<blockmirror::chain::scri::NewData>(n1);
    dt1.add(p1);

    blockmirror::store::NewDataPtr p2 = dt1.query(std::string("222"));
    if (p2 == nullptr) {
        std::cout<<"nullptr"<<std::endl;
    }
    else{
        std::cout<<p2->getName()<<"::"<<p2->getFormat()<<std::endl;
    }

    blockmirror::store::NewDataPtr p3 = dt1.query(std::string("111"));
    if (p3 == nullptr) {
        std::cout<<"nullptr"<<std::endl;
    }
    else{
        std::cout<<p3->getName()<<"::"<<p3->getFormat()<<std::endl;
    } 

    dt1.close();

    std::cout<<"data test end."<<std::endl;
}

BOOST_AUTO_TEST_SUITE_END()