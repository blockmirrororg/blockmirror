
#include <blockmirror/chain/context.h>
#include <blockmirror/chain/transaction.h>
#include <blockmirror/rpc/session.h>
#include <blockmirror/serialization/ptree_iarchive.h>
#include <blockmirror/store/data_store.h>
#include <blockmirror/store/transaction_store.h>
#include <boost/algorithm/hex.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace blockmirror {
namespace rpc {

Session::Session(tcp::socket socket)
    : socket_(std::move(socket)),
      strand_(socket_.get_executor()),
      lambda_(*this) {}

void Session::run() { do_read(); }

void Session::do_read() {
  req_ = {};

  http::async_read(
      socket_, buffer_, req_,
      boost::asio::bind_executor(
          strand_, std::bind(&Session::on_read, shared_from_this(),
                             std::placeholders::_1, std::placeholders::_2)));
}

void Session::on_read(boost::system::error_code ec,
                      std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);

  if (ec == http::error::end_of_stream)
    return do_close();

  if (ec)
    return;

  handle_request(std::move(req_), lambda_);
}

void Session::on_write(boost::system::error_code ec,
                       std::size_t bytes_transferred, bool close) {
  boost::ignore_unused(bytes_transferred);

  if (ec) return;

  if (close) return do_close();

  do_read();
}

void Session::do_close() {
  boost::system::error_code ec;
  socket_.shutdown(tcp::socket::shutdown_send, ec);
}

template <class Body, class Allocator, class Send>
void Session::handle_request(
    http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {

  auto const bad_request = [&req](boost::beast::string_view why) {
    http::response<http::string_body> res{http::status::bad_request,
                                          req.version()};
    res.keep_alive(req.keep_alive());
    res.body() = why.to_string();
    res.prepare_payload();
    return res;
  };
  auto const server_error = [&req](boost::beast::string_view what) {
    http::response<http::string_body> res{http::status::internal_server_error,
                                          req.version()};
    res.keep_alive(req.keep_alive());
    res.body() = "An error occurred: '" + what.to_string() + "'";
    res.prepare_payload();
    return res;
  };
  auto const ok_repeat = [&req](boost::beast::string_view what) {
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.keep_alive(req.keep_alive());
    res.body() = what.to_string();
    res.prepare_payload();
    return res;
  };

  if (req.method() != http::verb::post)  // put transaction and put data
    return send(bad_request("Unknown HTTP-method"));

  std::stringstream ss(req.body());
  boost::property_tree::ptree ptree;

  if (req.target() == "/put_transaction") {
    blockmirror::chain::TransactionSignedPtr transaction =
        std::make_shared<blockmirror::chain::TransactionSigned>();
    try {
      boost::property_tree::read_json(ss, ptree);
      blockmirror::serialization::PTreeIArchive archive(ptree);
      archive >> transaction;
    } catch (std::exception& e) {
      return send(server_error(e.what()));
    }
    chain::Context& c = chain::Context::get();
    if (!c.check(transaction)) {
      return send(bad_request("check failed"));
    }
    store::TransactionStore& ts = c.get_transaction_store();
    if (!ts.add(transaction)) {
      return send(ok_repeat("repeat put, modified!"));
    }

    http::response<http::empty_body> res{http::status::ok, req.version()};
    res.keep_alive(req.keep_alive());

    return send(std::move(res));

  } else if (req.target() == "/put_data") {
    blockmirror::chain::TransactionSignedPtr transaction =
      std::make_shared<blockmirror::chain::TransactionSigned>();
    try {
      boost::property_tree::read_json(ss, ptree);
      blockmirror::serialization::PTreeIArchive archive(ptree);
      archive >> transaction;
    }
    catch (std::exception& e) {
      return send(server_error(e.what()));
    }
    chain::Context& c = chain::Context::get();
    if (!c.check(transaction)) {
      return send(bad_request("check failed"));
    }
    store::TransactionStore& ts = c.get_transaction_store();
    if (!ts.add(transaction)) {
      return send(ok_repeat("repeat put, modified!"));
    }

  } else {
    return send(bad_request("Illegal request-target"));
  }
}

}  // namespace rpc
}  // namespace blockmirror