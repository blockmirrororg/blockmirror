#ifndef SERVER_H
#define SERVER_H

#include <blockmirror/rpc/listener.h>
#include <boost/noncopyable.hpp>
#include "Acceptor.h"
#include "Connector.h"

class Server : private boost::noncopyable {
 public:
  explicit Server(std::size_t thread_pool_size);

 public:
  void run();

 private:
  void handle_stop(int signo);
  void handle_timeout();

 private:
  boost::asio::io_context io_context_;   // for main thread
  boost::asio::io_context io_context1_;  // for work thread

  boost::asio::deadline_timer timer_;
  boost::asio::signal_set signals_;

  // p2p
  Acceptor acceptor_;
  Connector connector_;
  // rpc
  blockmirror::rpc::Listener listener_;

  std::size_t thread_pool_size_;
};

#endif