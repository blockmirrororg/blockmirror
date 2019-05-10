
#include <blockmirror/server.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

namespace blockmirror {

Server::Server(std::size_t thread_pool_size)
    : io_context_(),
      io_context1_(),
      acceptor_(io_context1_, 80),
      timer_(io_context_, boost::posix_time::seconds(10)),
      signals_(io_context_),
      listener_(
          io_context1_,
          boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 8080}),
      thread_pool_size_(thread_pool_size) {}

void Server::handle_stop(int signo) {
  io_context_.stop();
  io_context1_.stop();
}

void Server::handle_timeout() {
  // do something
	
  timer_.expires_at(timer_.expires_at() + boost::posix_time::seconds(10));
  timer_.async_wait(boost::bind(&Server::handle_timeout, this));
}

void Server::run() {
  signals_.add(SIGINT);
  signals_.add(SIGTERM);
#if defined(SIGQUIT)
  signals_.add(SIGQUIT);
#endif
  // main thread job
  signals_.async_wait(boost::bind(&Server::handle_stop, this,
                                  boost::asio::placeholders::signal_number));
  timer_.async_wait(boost::bind(&Server::handle_timeout, this));

  // work thread job
  // p2p
  acceptor_.start_accept();
  for (auto pos = connectors_.begin(); pos != connectors_.end(); ++pos) {
    (*pos)->start();
  }
  // rpc
  listener_.run();

  std::vector<boost::shared_ptr<std::thread>> threads;
  for (std::size_t i = 0; i < thread_pool_size_; ++i) {
    boost::shared_ptr<std::thread> thread(new std::thread(
        boost::bind(&boost::asio::io_context::run, &io_context1_)));
    threads.push_back(thread);
  }

  io_context_.run();
  for (std::size_t i = 0; i < thread_pool_size_; ++i) {
    threads[i]->join();
  }
}

void Server::add_connector(const char *ip, unsigned short port)
{
	connectors_.push_back(boost::make_shared<blockmirror::p2p::Connector>(io_context1_, ip, port));
}

}  // namespace blockmirror