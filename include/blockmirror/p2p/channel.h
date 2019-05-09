#pragma once

#include <blockmirror/p2p/binary_stream.h>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <deque>

namespace blockmirror {
namespace p2p {

class Channel : public boost::enable_shared_from_this<Channel>,
                   private boost::noncopyable {
 public:
  explicit Channel(boost::asio::io_context& ioc);

 public:
  void id(int channel_id);
  int id();
  boost::asio::ip::tcp::socket& socket();
  void start();
  Message& get_message();
  unsigned char remote_type() { return remote_type_; }
  void close();
  void send(const Message& message);

 private:
  void handle_read_header(const boost::system::error_code& e,
                          std::size_t bytes_transferred);
  void handle_read_body(const boost::system::error_code& e,
                        std::size_t bytes_transferred);
  void handle_write(const boost::system::error_code& e,
                    std::size_t bytes_transferred);

 private:
  boost::asio::ip::tcp::socket socket_;
  static const int HEADER_LEN = 8;
  Message message_;
  unsigned char remote_type_;
  boost::asio::detail::mutex mutex_;
  std::deque<Message> messages_;
  int channel_id_;
};

typedef boost::shared_ptr<Channel> channel_ptr;

}  // namespace p2p
}  // namespace blockmirror