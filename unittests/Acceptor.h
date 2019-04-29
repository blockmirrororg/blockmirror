#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include <boost/asio.hpp>
#include "Connection.h"

class Acceptor
{
public:
	Acceptor(boost::asio::io_service &s, unsigned short port);

private:
	void start_accept();
	void handle_accept(const boost::system::error_code& e);
	void handle_stop(int signo);

private:
	boost::asio::io_service &io_service_;
	boost::asio::ip::tcp::acceptor acceptor_;
	connection_ptr new_connection_;
};

#endif