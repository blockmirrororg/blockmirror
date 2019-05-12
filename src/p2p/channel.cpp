
#include <blockmirror/p2p/channel.h>
#include <blockmirror/p2p/handler_manager.h>
#include <boost/bind.hpp>

namespace blockmirror {
namespace p2p {

Channel::Channel(boost::asio::io_context& ioc)
    : socket_(ioc), remote_type_(0) {}

boost::asio::ip::tcp::socket& Channel::socket() { return socket_; }

void Channel::start() {
  boost::asio::async_read(
      socket_, boost::asio::buffer(message_.get_write_ptr(), HEADER_LEN),
      boost::bind(&Channel::handle_read_header, shared_from_this(),
                  boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred));
}

void Channel::handle_read_header(const boost::system::error_code& e,
                                 std::size_t bytes_transferred) {
  if (!e) {
    message_.move_write_pos(bytes_transferred);
    int body_len = message_.get_body_len();
    message_.move_read_pos(bytes_transferred);
    boost::asio::async_read(
        socket_, boost::asio::buffer(message_.get_write_ptr(), body_len),
        boost::bind(&Channel::handle_read_body, shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
  }
}

void Channel::handle_read_body(const boost::system::error_code& e,
                               std::size_t bytes_transferred) {
  if (!e) {
    message_.move_write_pos(bytes_transferred);

    HandlerManager::get().dispatch_message_handler(*this);
    message_.reset();
    boost::asio::async_read(
        socket_, boost::asio::buffer(message_.get_write_ptr(), HEADER_LEN),
        boost::bind(&Channel::handle_read_header, shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
  }
}

Message& Channel::get_message() { return message_; }

void Channel::close() {
  boost::system::error_code ignored_ec;
  socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
}

void Channel::send(const Message& message) {
  boost::asio::detail::mutex::scoped_lock lock(mutex_);
  if (messages_.empty()) {
    messages_.push_back(message);
    boost::asio::async_write(
        socket_,
        boost::asio::buffer(message_.get_read_ptr(), message_.get_readable()),
        boost::bind(&Channel::handle_write, shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
  } else {
    if (messages_.size() >= 1000) {
      close();
      return;
    }
    messages_.push_back(message);
  }
}

void Channel::handle_write(const boost::system::error_code& e,
                           std::size_t bytes_transferred) {
  if (!e) {
    boost::asio::detail::mutex::scoped_lock lock(mutex_);
    std::deque<Message>::iterator pos = messages_.begin();
    pos = messages_.erase(pos);
    if (pos != messages_.end()) {
      boost::asio::async_write(
          socket_,
          boost::asio::buffer(pos->get_read_ptr(), pos->get_readable()),
          boost::bind(&Channel::handle_write, shared_from_this(),
                      boost::asio::placeholders::error,
                      boost::asio::placeholders::bytes_transferred));
    }
  }
}

void Channel::id(int channel_id) { channel_id_ = channel_id; }

int Channel::id() { return channel_id_; }

}  // namespace p2p
}  // namespace blockmirror