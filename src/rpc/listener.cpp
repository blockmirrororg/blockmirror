
#include <blockmirror/rpc/listener.h>
#include <blockmirror/rpc/session.h>

namespace blockmirror {
namespace rpc {

Listener::Listener(boost::asio::io_context& ioc, tcp::endpoint endpoint, std::shared_ptr<std::string const> const& doc_root)
	: acceptor_(ioc), socket_(ioc), doc_root_(doc_root)
{
	boost::system::error_code ec;

	acceptor_.open(endpoint.protocol(), ec);
	if (ec)
	{
		return;
	}

	acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
	if (ec)
	{
		return;
	}

	acceptor_.bind(endpoint, ec);
	if (ec)
	{
		return;
	}

	acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
	if (ec)
	{
		return;
	}
}

void Listener::run()
{
	if (!acceptor_.is_open())
		return;
	do_accept();
}

void Listener::do_accept()
{
	acceptor_.async_accept(socket_, std::bind(&Listener::on_accept, shared_from_this(), std::placeholders::_1));
}

void Listener::on_accept(boost::system::error_code ec)
{
	if (!ec)
	{
		std::make_shared<Session>(std::move(socket_), doc_root_)->run();
	}
	
	do_accept();
}

}
}