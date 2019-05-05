#define BOOST_TEST_MAIN
#define BOOST_ASIO_ENABLE_OLD_SERVICES
//#include <boost/test/included/unit_test.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>
#include "Acceptor.h"
#include "Connector.h"

boost::asio::io_service s_;
boost::asio::io_service s1_;
boost::asio::deadline_timer timer_(s_, boost::posix_time::seconds(10)); // 10��󴥷�һ��

void handle_stop(int signo) {
	s_.stop();
	s1_.stop();
}

void handle_timeout() {
	// do something
	timer_.expires_at(timer_.expires_at() + boost::posix_time::seconds(10));
	timer_.async_wait(boost::bind(handle_timeout));
}

int main(int argc, char* argv[]) {
	try {
		// if (argc != 2) {
		//   std::cerr << "Usage: unit_test <workthreads>\n";
		   //return 1;
		// }

		std::size_t workthreads = 4;
		if (argc > 1) {
			workthreads = boost::lexical_cast<std::size_t>(argv[1]);
		}

		// ʹ��kill������ź��ó���ȫ�˳�
		boost::asio::signal_set signals(s_);
		signals.add(SIGINT);
		signals.add(SIGTERM);
#if defined(SIGQUIT)
		signals.add(SIGQUIT);
#endif
		// main thread job
		signals.async_wait(boost::bind(&handle_stop, boost::asio::placeholders::signal_number));
		timer_.async_wait(boost::bind(handle_timeout));

		// work thread job
		Acceptor acceptor(s1_, 8080); // �󶨵Ķ˿�
		Connector connector(s1_);

		std::vector<boost::shared_ptr<boost::thread>> threads;
		for (std::size_t i = 0; i < workthreads; ++i) {
			boost::shared_ptr<boost::thread> thread(new boost::thread(boost::bind(&boost::asio::io_service::run, &s1_)));
			threads.push_back(thread);
		}

		s_.run();
		for (std::size_t i = 0; i < workthreads; ++i) {
			threads[i]->join();
		}
	}
	catch (std::exception& e) {
		std::cerr << "exception: " << e.what() << "\n";
	}

	return 0;
} 