#ifndef CONNECTOR_H
#define CONNECTOR_H

#include <boost/asio.hpp>

class Connector
{
public:
	Connector(boost::asio::io_service& s, /*const char* ip, unsigned short port, */unsigned char remote_type = 0, unsigned char local_type = 0);

	void start(bool now = true);

private:
	void handle_connect(const boost::system::error_code &ec);
	void handle_write(const boost::system::error_code &ec);
	void handle_timer();
	void handle_resolve(const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);

private:
	boost::asio::ip::tcp::socket socket_;
	//boost::asio::ip::tcp::endpoint endpoint_;
	boost::asio::ip::tcp::resolver resolver_;

	boost::asio::deadline_timer timer_;
	boost::asio::io_service& io_service_;
	unsigned char remote_type_;
	unsigned char local_type_;
};

#endif