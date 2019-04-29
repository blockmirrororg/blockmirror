#include<boost/test/unit_test.hpp>
#include<blockmirror/store/format_store.h>

BOOST_AUTO_TEST_SUITE(format_tests)

BOOST_AUTO_TEST_CASE(format_tests_ok) {

    blockmirror::store::FormatStore ft1;
    boost::filesystem::path p("/ze");
    ft1.load(p);

    std::vector<uint8_t> v{1,2,3};
    blockmirror::chain::scri::NewFormat n1
    {std::string{"111"},std::string{"222"},v,v,v};
    blockmirror::store::NewFormatPtr p1 = 
    std::make_shared<blockmirror::chain::scri::NewFormat>(n1);
    ft1.add(p1);

    blockmirror::store::NewFormatPtr p2 = ft1.query(std::string("222"));
    if (p2 == nullptr) {
        std::cout<<"nullptr"<<std::endl;
    }
    else{
        std::cout<<p2->getName()<<std::endl;
    }

    blockmirror::store::NewFormatPtr p3 = ft1.query(std::string("111"));
    if (p3 == nullptr) {
        std::cout<<"nullptr"<<std::endl;
    }
    else{
        std::cout<<p3->getName()<<std::endl;
    }

    ft1.close();

    std::cout<<"format test end."<<std::endl;
}

BOOST_AUTO_TEST_SUITE_END()