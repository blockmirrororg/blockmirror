
#include <boost/asio.hpp>
#include <blockmirror/server.h>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//#include <blockmirror/common.h>

namespace blockmirror {

Server::Server()
    : _mainContext(),
      _workContext(),
      _p2pAcceptor(_workContext, blockmirror::globalConfig.p2p_bind),
      _signals(_mainContext),
      _context(),
      _rpcListener(
          _workContext,
          boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), blockmirror::globalConfig.rpc_bind},
          _context) {}

void Server::handleSignals(int signo) {
  B_LOG("got signo {}", signo);
  _mainContext.stop();
  _workContext.stop();
}

void Server::stop() {
  _mainContext.stop();
  _workContext.stop();
}

void Server::createBlock(const boost::system::error_code& e,
                         boost::asio::deadline_timer* t) {
  blockmirror::chain::BlockPtr head = _context.getHead();
  int mySlot = -1;
  uint64_t nowSlot = 0;
  if (head) {
    mySlot = _context.getBpsStore().find(globalConfig.miner_pubkey);
    nowSlot = _context.getBpsStore().getSlotByTime(now_ms_since_1970());
  } else {
    mySlot = nowSlot;
  }

  if (mySlot == nowSlot) {
    blockmirror::chain::BlockPtr block = _context.genBlock(
        globalConfig.miner_privkey, globalConfig.reward_pubkey);
  }

  int delay = _context.getBpsStore().getBPDelay(globalConfig.miner_pubkey);
  t->expires_at(t->expires_at() + boost::posix_time::millisec(delay));
  t->async_wait(boost::bind(&Server::createBlock, this,
                            boost::asio::placeholders::error, t));
}

void Server::run() {

  for (auto pos = blockmirror::globalConfig.seeds.begin();
       pos != blockmirror::globalConfig.seeds.end(); ++pos) {
    char ip[50] = {0};
    std::size_t index = pos->find(':');
    if (index == std::string::npos) {
      throw std::runtime_error("error seeds");
    }
    strncpy(ip, pos->c_str(), index);
    unsigned short port =
        boost::lexical_cast<unsigned short>(pos->c_str() + index + 1);
    boost::shared_ptr<blockmirror::p2p::Connector> connector =
        boost::make_shared<blockmirror::p2p::Connector>(_workContext, ip, port);
    connector->start();
    _p2pConnecting.push_back(connector);
  }

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

  int delay = _context.getBpsStore().getBPDelay(globalConfig.miner_pubkey);
  boost::asio::deadline_timer timer(_mainContext,
                                    boost::posix_time::millisec(delay));
  timer.async_wait(boost::bind(&Server::createBlock, this,
                               boost::asio::placeholders::error, &timer));

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

}  // namespace blockmirror