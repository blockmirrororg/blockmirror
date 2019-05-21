#ifndef SERVER_H
#define SERVER_H

#include <blockmirror/chain/context.h>
#include <blockmirror/p2p/acceptor.h>
#include <blockmirror/p2p/connector.h>
#include <blockmirror/rpc/listener.h>
#include <boost/noncopyable.hpp>
#include <list>

namespace blockmirror {

class Server : private boost::noncopyable {
 public:
  explicit Server();

 public:
  void run();
  void stop();
  static Server& get()
  {
	  static Server s;
	  return s;
  }

 private:
  void handleSignals(int signo);
  void produceBlock(const boost::system::error_code& e);
  void nextProduce();

 private:
  boost::asio::io_context _mainContext;  // for main thread
  boost::asio::io_context _workContext;  // for work thread

  boost::asio::signal_set _signals;

  blockmirror::chain::Context _context;
  boost::asio::deadline_timer _producerTimer;

  // p2p
  blockmirror::p2p::Acceptor _p2pAcceptor;
  std::list<boost::shared_ptr<blockmirror::p2p::Connector> > _p2pConnecting;
  // rpc
  blockmirror::rpc::Listener _rpcListener;
};

}  // namespace blockmirror

#endif