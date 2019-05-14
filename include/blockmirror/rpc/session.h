#ifndef SESSION_H
#define SESSION_H

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/config.hpp>
#include <blockmirror/chain/context.h>

namespace blockmirror {
namespace rpc {

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

class Session : public std::enable_shared_from_this<Session> {
  struct send_lambda {
    Session& self_;

    explicit send_lambda(Session& self) : self_(self) {}

    template <bool isRequest, class Body, class Fields>
    void operator()(http::message<isRequest, Body, Fields>&& msg) const {
      auto sp = std::make_shared<http::message<isRequest, Body, Fields>>(
          std::move(msg));
      self_.res_ = sp;

      http::async_write(
          self_.socket_, *sp,
          boost::asio::bind_executor(
              self_.strand_,
              std::bind(&Session::on_write, self_.shared_from_this(),
                        std::placeholders::_1, std::placeholders::_2,
                        !sp->keep_alive())));
    }
  };

  tcp::socket socket_;
  boost::asio::strand<tcp::socket::executor_type> strand_;
  boost::beast::flat_buffer buffer_;
  http::request<http::string_body> req_;
  std::shared_ptr<void> res_;
  send_lambda lambda_;
  blockmirror::chain::Context &_context;

 public:
  explicit Session(tcp::socket socket, blockmirror::chain::Context &context);

  void run();
  void do_read();
  void on_read(boost::system::error_code ec, std::size_t bytes_transferred);
  void on_write(boost::system::error_code ec, std::size_t bytes_transferred,
                bool close);
  void do_close();

 private:
  template <class Body, class Allocator, class Send>
  void handle_request(http::request<Body, http::basic_fields<Allocator> >&& req,
                      Send&& send);
  template<class Body, class Allocator, class Send>
  void deal_post(http::request<Body, http::basic_fields<Allocator> >&&req, Send&& send);
  
  template<class Body, class Allocator, class Send>
  void deal_get (http::request<Body, http::basic_fields<Allocator> >&&req, Send&& send);

  //char* strstr(const char* string, const char* find);
  int getUrlencodedValue(const char* data, char* item, int maxSize, char* val);
};

}  // namespace rpc
}  // namespace blockmirror

#endif