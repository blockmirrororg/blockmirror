#ifndef CONNECTOR_H
#define CONNECTOR_H

#include <boost/asio.hpp>

class Connector
{
public:
	Connector(boost::asio::io_service& s, const char* ip, unsigned short port, unsigned char remote_type, unsigned char local_type);

	void start(bool now = true);

private:
	void handle_connect(const boost::system::error_code &ec);
	void handle_write(const boost::system::error_code &ec);
	void handle_timer();

private:
	boost::asio::ip::tcp::socket socket_;
	boost::asio::ip::tcp::endpoint endpoint_;
	boost::asio::deadline_timer timer_;
	boost::asio::io_service& io_service_;
	unsigned char remote_type_;
};

#endif