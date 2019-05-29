#ifndef SESSION_H
#define SESSION_H

#include <blockmirror/chain/context.h>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/config.hpp>

namespace blockmirror {
namespace rpc {

using tcp = boost::asio::ip::tcp;
namespace beast = boost::beast;
namespace http = boost::beast::http;

class Session : public std::enable_shared_from_this<Session> {
  struct send_lambda {
    Session& self_;

    explicit send_lambda(Session& self) : self_(self) {}

    template <bool isRequest, class Body, class Fields>
    void operator()(http::message<isRequest, Body, Fields>&& msg, bool stopService = false) const {
      msg.keep_alive(true);
      auto sp = std::make_shared<http::message<isRequest, Body, Fields>>(
          std::move(msg));
      self_.res_ = sp;
      B_LOG("replay {}", sp->need_eof());

      /*http::async_write(
          self_.stream_, *sp,
          beast::bind_front_handler(&Session::on_write,
                                    self_.shared_from_this(), sp->need_eof()));*/
      http::async_write(
          self_.socket_, *sp,
          boost::asio::bind_executor(
              self_.strand_,
              std::bind(&Session::on_write, self_.shared_from_this(),
                        std::placeholders::_1, std::placeholders::_2,
                        !sp->keep_alive(), stopService)));
    }
  };

  //boost::beast::tcp_stream stream_;
  tcp::socket socket_;
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;

  boost::beast::flat_buffer buffer_;
  http::request<http::string_body> req_;
  std::shared_ptr<void> res_;
  send_lambda lambda_;
  blockmirror::chain::Context& _context;
  beast::string_view doc_root = blockmirror::globalConfig.rpc_resource;

 private:
  friend class Listener;
  typedef void (Session::*PostMethodFuncPtr)();
  typedef void (Session::*GetMethodFuncPtr)(const char*);
  static std::map<std::string, GetMethodFuncPtr> _getMethodPtrs;
  static std::map<std::string, PostMethodFuncPtr> _postMethodPtrs;

  static GetMethodFuncPtr getMethodFuncPtr(const char* target);
  static PostMethodFuncPtr postMethodFuncPtr(const char* target);

  http::response<http::string_body> bad_request(std::string why);
  http::response<http::string_body> server_error(std::string what);
  http::response<http::string_body> ok(std::string what);

 public:
  explicit Session(tcp::socket socket, blockmirror::chain::Context& context, boost::asio::io_context &ioc);

  void run();
  void do_read();
  void on_read(boost::system::error_code ec, std::size_t bytes_transferred);
  void on_write(beast::error_code ec, std::size_t bytes_transferred,
                bool close, bool stopService);
  void do_close();

  // post
  void postChainTransaction();
  void postChainData();
  // get
  void getNodeStop(const char*);
  void getNodeVersion(const char*);
  void getNodePeers(const char*);
  void getNodeConnect(const char*);
  void getChainStatus(const char*);
  void getChainLast(const char*);
  void getChainBlock(const char*);
  void getChainTransaction(const char*);
  void getChainFormat(const char*);
  void getChainDatatypes(const char*);
  void getChainBps(const char*);

 private:
  void handle_request();

  int getUrlencodedValue(const char* data, char* item, int maxSize, char* val);

  void handle_file(http::request<http::string_body>&& req);

  beast::string_view mime_type(beast::string_view path);

  std::string path_cat(beast::string_view base, beast::string_view path);
};

}  // namespace rpc
}  // namespace blockmirror

#endif