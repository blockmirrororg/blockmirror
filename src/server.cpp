
#include <boost/asio.hpp>
#include <blockmirror/server.h>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

namespace blockmirror {

Server::Server()
    : _mainContext(),
      _workContext(),
      _p2pAcceptor(_workContext, 80),
      _signals(_mainContext),
      _context(*this),
      _rpcListener(
          _workContext,
          boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 8080},
          _context) {}

void Server::handleSignals(int signo) {
  std::cout << "got signal: " << signo << std::endl;
  _mainContext.stop();
  _workContext.stop();
}

void Server::stop()
{
	std::cout << "received rpc target: /node/stop" << std::endl;
	_mainContext.stop();
	_workContext.stop();
}

void Server::run() {
  _signals.add(SIGINT);
  _signals.add(SIGTERM);
#if defined(SIGQUIT)
  _signals.add(SIGQUIT);
#endif
  // main thread job
  _signals.async_wait(boost::bind(&Server::handleSignals, this,
                                  boost::asio::placeholders::signal_number));

  _context.load();

  // FIXME: Debug only
  // _context.debugInit();

  // rpc
  _rpcListener.run();

  std::vector<boost::shared_ptr<std::thread>> threads;
  for (std::size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
    boost::shared_ptr<std::thread> thread(new std::thread(
        boost::bind(&boost::asio::io_context::run, &_workContext)));
    threads.push_back(thread);
  }

  try {
    _mainContext.run();
  } catch (...) {
    std::cerr << "got unknown exception" << std::endl;
  }

  for (auto thread : threads) {
    thread->join();
  }

  _context.close();
}

void Server::add_connector(const char *ip, unsigned short port) {
  _p2pConnecting.push_back(
      boost::make_shared<blockmirror::p2p::Connector>(_workContext, ip, port));
}

}  // namespace blockmirror