
#include "Acceptor.h"
#include <boost/bind.hpp>

Acceptor::Acceptor(boost::asio::io_service &s, unsigned short port)
	: io_service_(s), acceptor_(io_service_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{
	start_accept();
}

void Acceptor::start_accept()
{
	new_connection_.reset(new Connection(io_service_));
	acceptor_.async_accept(new_connection_->socket(), boost::bind(&Acceptor::handle_accept, this, boost::asio::placeholders::error));
}

void Acceptor::handle_accept(const boost::system::error_code& e)
{
	if (!e)
	{
		new_connection_->start();
	}

	start_accept();
}