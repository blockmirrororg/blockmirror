
#include <boost/asio.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>
#include "Acceptor.h"
#include "Connector.h"
#include <blockmirror/rpc/listener.h>

boost::asio::io_context io_context_;
boost::asio::io_context io_context1_;
boost::asio::deadline_timer timer_(
    io_context_, boost::posix_time::seconds(10));  // 10秒后触发一次

void handle_stop(int signo) {
  io_context_.stop();
  io_context1_.stop();
}

void handle_timeout() {
  // do something
  timer_.expires_at(timer_.expires_at() + boost::posix_time::seconds(10));
  timer_.async_wait(boost::bind(handle_timeout));
}

int main(int argc, char* argv[]) {
  try {
    std::size_t workthreads = 4;
    auto doc_root = std::make_shared<std::string>("");
    if (argc == 3) {
      workthreads = boost::lexical_cast<std::size_t>(argv[1]);
      doc_root = std::make_shared<std::string>(argv[2]);
    }

    boost::asio::signal_set signals(io_context_);
    signals.add(SIGINT);
    signals.add(SIGTERM);
#if defined(SIGQUIT)
    signals.add(SIGQUIT);
#endif
    // main thread job
    signals.async_wait(
        boost::bind(&handle_stop, boost::asio::placeholders::signal_number));
    timer_.async_wait(boost::bind(handle_timeout));

    // work thread job
    // p2p
    Acceptor acceptor(io_context1_, 80);
    Connector connector(io_context1_);
    // rpc
    std::make_shared<blockmirror::rpc::Listener>(
        io_context1_,
        boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 8080},
        doc_root)
        ->run();

    std::vector<boost::shared_ptr<boost::thread>> threads;
    for (std::size_t i = 0; i < workthreads; ++i) {
      boost::shared_ptr<boost::thread> thread(new boost::thread(
          boost::bind(&boost::asio::io_context::run, &io_context1_)));
      threads.push_back(thread);
    }

    io_context_.run();
    for (std::size_t i = 0; i < workthreads; ++i) {
      threads[i]->join();
    }
  } catch (std::exception& e) {
    std::cerr << "exception: " << e.what() << "\n";
  }

  return 0;
}