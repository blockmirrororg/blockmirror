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
namespace http = boost::beast::http;

class Session : public std::enable_shared_from_this<Session> {

  struct send_lambda {
    Session& self_;

    explicit send_lambda(Session& self) : self_(self) {}

    template <bool isRequest, class Body, class Fields>
    void operator()(http::message<isRequest, Body, Fields>&& msg,
                    bool stopService = false) const {
      auto sp = std::make_shared<http::message<isRequest, Body, Fields>>(
          std::move(msg));
      self_.res_ = sp;

      http::async_write(
          self_.socket_, *sp,
          boost::asio::bind_executor(
              self_.strand_,
              std::bind(&Session::on_write, self_.shared_from_this(),
                        std::placeholders::_1, std::placeholders::_2,
                        !sp->keep_alive(), stopService)));
    }
  };

  tcp::socket socket_;
  boost::asio::strand<tcp::socket::executor_type> strand_;
  boost::beast::flat_buffer buffer_;
  http::request<http::string_body> req_;
  std::shared_ptr<void> res_;
  send_lambda lambda_;
  blockmirror::chain::Context& _context;

 private:
  friend class Listener;
  typedef void (Session::*PostMethodFuncPtr)();
  typedef void (Session::*GetMethodFuncPtr)(const char*);
  static std::map<std::string, GetMethodFuncPtr> _getMethodPtrs;
  static std::map<std::string, PostMethodFuncPtr> _postMethodPtrs;

  static GetMethodFuncPtr getMethodFuncPtr(const char* target) {
    auto pos = _getMethodPtrs.find(target);
    if (pos != _getMethodPtrs.end()) {
      return pos->second;
    } else {
      return nullptr;
    }
  }
  static PostMethodFuncPtr postMethodFuncPtr(const char* target) {
    auto pos = _postMethodPtrs.find(target);
    if (pos != _postMethodPtrs.end()) {
      return pos->second;
    } else {
      return nullptr;
    }
  }

 public:
  explicit Session(tcp::socket socket, blockmirror::chain::Context& context);

  void run();
  void do_read();
  void on_read(boost::system::error_code ec, std::size_t bytes_transferred);
  void on_write(boost::system::error_code ec, std::size_t bytes_transferred,
                bool close, bool stopService);
  void do_close();

  // post
  void postChainTransaction();
  void postPutData();
  // get
  void getNodeStop(const char*);
  void getNodeVersion(const char*);
  void getNodePeers(const char*);
  void getNodeConnect(const char*);
  void getChainStatus(const char*);
  void getChainLast(const char*);
  void getChainBlock(const char*);
  void getChainTransaction(const char*);

 private:
  void handle_request(http::request<http::string_body>&& req);

  int getUrlencodedValue(const char* data, char* item, int maxSize, char* val);
};

}  // namespace rpc
}  // namespace blockmirror

#endif