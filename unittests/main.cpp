#define BOOST_TEST_MAIN
#define BOOST_ASIO_ENABLE_OLD_SERVICES
//#include <boost/test/included/unit_test.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>
#include "Acceptor.h"

boost::asio::io_service s_;
boost::asio::io_service s1_;

void handle_stop(int signo) {
	s_.stop();
	s1_.stop();
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

		// 使用kill命令发送信号让程序安全退出
		boost::asio::signal_set signals(s_);
		signals.add(SIGINT);
		signals.add(SIGTERM);
#if defined(SIGQUIT)
		signals.add(SIGQUIT);
#endif
		signals.async_wait(boost::bind(&handle_stop, boost::asio::placeholders::signal_number));

		Acceptor acceptor(s1_, 8080); // 绑定的端口

		std::vector<boost::shared_ptr<boost::thread>> threads;
		for (std::size_t i = 0; i < workthreads; ++i) {
			boost::shared_ptr<boost::thread> thread(
				new boost::thread(boost::bind(&boost::asio::io_service::run, &s1_)));
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