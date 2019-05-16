
#include <blockmirror/rpc/listener.h>
#include <blockmirror/rpc/session.h>

namespace blockmirror {
namespace rpc {

Listener::Listener(boost::asio::io_context& ioc, tcp::endpoint endpoint, blockmirror::chain::Context &context)
    : acceptor_(ioc), socket_(ioc), _context(context) {

	/*HttpHandler& httpHandler = HttpHandler::get();
	httpHandler.register_target("/node/stop", new GetNodeStop);
	httpHandler.register_target("/node/version", new GetNodeVersion);
	httpHandler.register_target("/node/Peers", new GetNodePeers);
	httpHandler.register_target("/chain/status", new GetChainStatus);
	httpHandler.register_target("/chain/last", new GetChainLast);
	httpHandler.register_target("/chain/block", new GetChainBlock);
	httpHandler.register_target("/chain/transaction", new GetChainTransaction);
	httpHandler.register_target("/node/connect", new GetNodeConnect);
	httpHandler.register_target("/put_transaction", new PostPutData);
	httpHandler.register_target("/put_data", new PutTransaction);*/
	Session::_getMethodDeals.insert(std::make_pair("/node/stop", &Session::getNodeStop));
	Session::_getMethodDeals.insert(std::make_pair("/node/version", &Session::getNodeVersion));
	Session::_getMethodDeals.insert(std::make_pair("/node/peers", &Session::getNodePeers));
	Session::_getMethodDeals.insert(std::make_pair("/chain/status", &Session::getChainStatus));
	Session::_getMethodDeals.insert(std::make_pair("/chain/last", &Session::getChainLast));
	Session::_getMethodDeals.insert(std::make_pair("/chain/block", &Session::getChainBlock));
	Session::_getMethodDeals.insert(std::make_pair("/chain/transaction", &Session::getChainTransaction));
	Session::_postMethodDeals.insert(std::make_pair("/put_transaction", &Session::postPutTransaction));
	Session::_postMethodDeals.insert(std::make_pair("/chain/transaction", &Session::postPutData));

  boost::system::error_code ec;
  acceptor_.open(endpoint.protocol(), ec);
  if (ec) {
    return;
  }

  acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
  if (ec) {
    return;
  }

  acceptor_.bind(endpoint, ec);
  if (ec) {
    return;
  }

  acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
  if (ec) {
    return;
  }
}

void Listener::run() {
  if (!acceptor_.is_open()) return;
  do_accept();
}

void Listener::do_accept() {
  acceptor_.async_accept(socket_,
                         std::bind(&Listener::on_accept, this,
                                   std::placeholders::_1));
}

void Listener::on_accept(boost::system::error_code ec) {
  if (!ec) {
    std::make_shared<Session>(std::move(socket_), _context)->run();
  }

  do_accept();
}

}  // namespace rpc
}  // namespace blockmirror