#ifndef CONNECTION_H
#define CONNECTION_H

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "Message.h"
#include <deque>

class Connection :public boost::enable_shared_from_this<Connection>, private boost::noncopyable
{
public:
	explicit Connection(boost::asio::io_service &s);

public:
	boost::asio::ip::tcp::socket& socket();
	void start();
	Message& get_message();
	unsigned char remote_type() { return remote_type_; }
	void close();
	void send(const Message& message);

private:
	void handle_read_header(const boost::system::error_code& e, std::size_t bytes_transferred);
	void handle_read_body(const boost::system::error_code& e, std::size_t bytes_transferred);
	void handle_write(const boost::system::error_code& e, std::size_t bytes_transferred);

private:
	boost::asio::ip::tcp::socket socket_;
	static const int HEADER_LEN = 8;
	Message message_;
	unsigned char remote_type_;
	boost::asio::detail::mutex mutex_;
	std::deque<Message> messages_;
};

typedef boost::shared_ptr<Connection> connection_ptr;

#endif