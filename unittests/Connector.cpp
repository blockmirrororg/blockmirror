
#include "Connector.h"
#include <boost/bind.hpp>

Connector::Connector(boost::asio::io_service& s, const char* ip, unsigned short port, unsigned char remote_type, unsigned char local_type)
	:socket_(s), endpoint_(boost::asio::ip::address::from_string(ip), port), timer_(s), io_service_(s), remote_type_(remote_type)
{
	start();
}

void Connector::start(bool now)
{
	if (now)
	{
		socket_.async_connect(endpoint_, boost::bind(&Connector::handle_connect, this, boost::asio::placeholders::error));
	}
	else
	{
		timer_.expires_from_now(boost::posix_time::seconds(5));
		timer_.async_wait(boost::bind(&Connector::handle_timer, this));
	}
}

void Connector::handle_connect(const boost::system::error_code &ec)
{
	if (!ec)
	{

	}
	else
	{
		socket_.close();
		timer_.expires_from_now(boost::posix_time::seconds(5));
		timer_.async_wait(boost::bind(&Connector::handle_timer, this));
	}
}

void Connector::handle_timer()
{
	socket_.async_connect(endpoint_, boost::bind(&Connector::handle_connect, this, boost::asio::placeholders::error));
}