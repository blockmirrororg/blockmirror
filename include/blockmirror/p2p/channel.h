#pragma once

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <deque>
#include <blockmirror/p2p/message.h>

namespace blockmirror {
namespace p2p {

class Channel : public boost::enable_shared_from_this<Channel>,
                   private boost::noncopyable {
 public:
  explicit Channel(boost::asio::io_context& ioc);

 public:
  void id(int channel_id);
  int id();
  boost::asio::ip::tcp::socket& socket() { return _socket; }
  void start();
  unsigned char remote_type() { return _remoteType; }
  void close();
  void send(const Message& message);

 private:
  void handleReadHeader(const boost::system::error_code& e,
                          std::size_t bytes_transferred);
  void handle_read_body(const boost::system::error_code& e,
                        std::size_t bytes_transferred);
  void handle_write(const boost::system::error_code& e,
                    std::size_t bytes_transferred);

 private:
  std::array<uint8_t, 1024> _buf;
  int _writePos;
  boost::asio::ip::tcp::socket _socket;
  unsigned char _remoteType;
  boost::asio::detail::mutex _mutex;
  int _channelId;
};

typedef boost::shared_ptr<Channel> channel_ptr;

}  // namespace p2p
}  // namespace blockmirror