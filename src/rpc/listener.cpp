
#include <blockmirror/rpc/listener.h>
#include <blockmirror/rpc/session.h>
#include <boost/asio/strand.hpp>

namespace blockmirror {
namespace rpc {

Listener::Listener(boost::asio::io_context& ioc, tcp::endpoint endpoint,
                   blockmirror::chain::Context& context)
    : acceptor_(ioc), socket_(ioc), _context(context), _ioc(ioc) {  

  Session::_getMethodPtrs.insert(
      std::make_pair("/node/stop", &Session::getNodeStop));
  Session::_getMethodPtrs.insert(
      std::make_pair("/node/version", &Session::getNodeVersion));
  Session::_getMethodPtrs.insert(
      std::make_pair("/node/peers", &Session::getNodePeers));
  Session::_getMethodPtrs.insert(
      std::make_pair("/chain/status", &Session::getChainStatus));
  Session::_getMethodPtrs.insert(
      std::make_pair("/chain/last", &Session::getChainLast));
  Session::_getMethodPtrs.insert(
      std::make_pair("/chain/block", &Session::getChainBlock));
  Session::_getMethodPtrs.insert(
      std::make_pair("/chain/transaction", &Session::getChainTransaction));
  Session::_getMethodPtrs.insert(
      std::make_pair("/node/connect", &Session::getNodeConnect));
  Session::_getMethodPtrs.insert(
      std::make_pair("/chain/format", &Session::getChainFormat));
  Session::_getMethodPtrs.insert(
      std::make_pair("/chain/datatypes", &Session::getChainDatatypes));
  Session::_getMethodPtrs.insert(
      std::make_pair("/chain/bps", &Session::getChainBps));
  Session::_getMethodPtrs.insert(
      std::make_pair("/chain/formats", &Session::getChainFormats));
  Session::_getMethodPtrs.insert(
      std::make_pair("/chain/datatype", &Session::getChainDatatype));

  Session::_postMethodPtrs.insert(
      std::make_pair("/chain/transaction", &Session::postChainTransaction));
  Session::_postMethodPtrs.insert(
      std::make_pair("/chain/data", &Session::postChainData));

  boost::system::error_code ec;
  acceptor_.open(endpoint.protocol(), ec);
  if (ec) {
    B_ERR("open {}", ec.message());
    return;
  }

	// 另外一个进程绑走了该端口则accept无法接收；不设置该选项有时会出现进程退出了，端口还处于被占用的状态
  /*acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
  if (ec) {
    B_ERR("set_option {}", ec.message());
    return;
  }*/

  acceptor_.bind(endpoint, ec);
  if (ec) {
    /*B_ERR("bind {}", ec.message());
    return;*/
		throw std::runtime_error("rpc绑定端口失败");
  }

  acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
  if (ec) {
    B_ERR("listen {}", ec.message());
    return;
  }
}

void Listener::run() {
  if (!acceptor_.is_open()) {
    throw std::runtime_error("!acceptor_.is_open()");
  }
  do_accept();
}

void Listener::do_accept() {
  /*acceptor_.async_accept(
      boost::asio::make_strand(acceptor_.get_executor()),
      boost::beast::bind_front_handler(&Listener::on_accept, this));*/
  acceptor_.async_accept(socket_, std::bind(&Listener::on_accept, this, std::placeholders::_1));
}

void Listener::on_accept(boost::system::error_code ec/*, tcp::socket socket*/) {
  if (!ec) {
    B_LOG("new connection {}", socket_.remote_endpoint().address().to_string());
    std::make_shared<Session>(std::move(socket_), _context, _ioc)->run();
  } else {
    B_ERR("accept error {}", ec.message());
  }

  do_accept();
}

}  // namespace rpc
}  // namespace blockmirror