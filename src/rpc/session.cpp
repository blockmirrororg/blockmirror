
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

Session::Session(tcp::socket socket, blockmirror::chain::Context& context)
    : socket_(std::move(socket)),
      strand_(socket_.get_executor()),
      lambda_(*this),
      _context(context) {}

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

  if (ec == http::error::end_of_stream) return do_close();

  if (ec) return;

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
    http::request<Body, http::basic_fields<Allocator> >&& req, Send&& send) {

  auto const bad_request = [&req](boost::beast::string_view why) {
    http::response<http::string_body> res{ http::status::bad_request,
                                          req.version() };
    res.keep_alive(req.keep_alive());
    res.body() = why.to_string();
    res.prepare_payload();
    return res;
  };

  if (req.method() == http::verb::post)
  {
    deal_post(std::move(req_), send);
  }
  else if (req.method() == http::verb::get)
  {
    deal_get(std::move(req_), send);
  }
  else
  {
    return send(bad_request("Illegal request-method"));
  }  
}

template<class Body, class Allocator, class Send>
void Session::deal_post(http::request<Body, http::basic_fields<Allocator> >&&req, Send&& send)
{
  auto const bad_request = [&req](boost::beast::string_view why) {
    http::response<http::string_body> res{ http::status::bad_request,
                                          req.version() };
    res.keep_alive(req.keep_alive());
    res.body() = why.to_string();
    res.prepare_payload();
    return res;
  };
  auto const server_error = [&req](boost::beast::string_view what) {
    http::response<http::string_body> res{ http::status::internal_server_error,
                                          req.version() };
    res.keep_alive(req.keep_alive());
    res.body() = "An error occurred: '" + what.to_string() + "'";
    res.prepare_payload();
    return res;
  };
  auto const ok_repeat = [&req](boost::beast::string_view what) {
    http::response<http::string_body> res{ http::status::ok, req.version() };
    res.keep_alive(req.keep_alive());
    res.body() = what.to_string();
    res.prepare_payload();
    return res;
  };

  std::stringstream ss(req.body());
  boost::property_tree::ptree ptree;

  chain::Context& c = _context;

  if (req.target() == "/put_transaction") {
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
    if (!c.check(transaction)) {
      return send(bad_request("check failed"));
    }
    store::TransactionStore& ts = c.getTransactionStore();
    if (!ts.add(transaction)) {
      return send(ok_repeat("repeat put, modified!"));
    }
  }

  else if (req.target() == "/put_data") {
    blockmirror::chain::TransactionSignedPtr transaction =
        std::make_shared<blockmirror::chain::TransactionSigned>();
    try {
      boost::property_tree::read_json(ss, ptree);
      blockmirror::serialization::PTreeIArchive archive(ptree);
      archive >> transaction;
    } catch (std::exception& e) {
      return send(server_error(e.what()));
    }
    if (!c.check(transaction)) {
      return send(bad_request("check failed"));
    }

    store::DataSignatureStore& dss = c.getDataSignatureStore();
    store::NewDataPtr newData = std::make_shared<chain::scri::NewData>(
      boost::get<chain::scri::NewData>(transaction->getScript()));
    if (!dss.add(newData)) {
      return send(ok_repeat("repeat put, modified!"));
    }
  }
  else if (req.target() == "/chain/transaction")
  {

  }
  else {
    return send(bad_request("Illegal request-target"));
  }

  http::response<http::empty_body> res{ http::status::ok, req.version() };
  res.keep_alive(req.keep_alive());
  return send(std::move(res));
}

template<class Body, class Allocator, class Send>
void Session::deal_get(http::request<Body, http::basic_fields<Allocator> >&&req, Send&& send) {
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
  auto const ok = [&req](boost::beast::string_view what) {
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.keep_alive(req.keep_alive());
    res.body() = what.to_string();
    res.prepare_payload();
    return res;
  };

  std::string strTarget = req.target().to_string();
  const char* target = strTarget.c_str();
  const char* ret = strchr(target, '?');
  if (!ret) {
    // 不带参数
    if (strcmp(target, "/node/stop") == 0) {
      return send(ok("{}"));
    } else if (strcmp(target, "/node/version") == 0) {
      return send(ok("{\"version\":0}"));
    } else if (strcmp(target, "/node/peers") == 0) {
      return send(ok("{\"connected\":[\"host\":\"127.0.0.1\",\"port\":8080]}"));
    } else if (strcmp(target, "/chain/status") == 0) {
      return send(ok("{\"height\":\"context.head.getHeight\",}"));
    } else if (strcmp(target, "/chain/last") == 0) {
    } else if (strcmp(target, "/chain/block/{HASH}") == 0) {
    } else if (strcmp(target, "/chain/transaction/{HASH}") == 0) {
    } else {
      return send(bad_request("{\"error\",\"Illegal request-target\"}"));
    }
  }
  else {
    // 带参数
    if (strncmp(target, "/node/connect", ret - target) == 0)
    {
      char host[50];
      char port[50];
      getUrlencodedValue(ret + 1, "host", sizeof(host) - 1, host);
      getUrlencodedValue(ret + 1, "port", sizeof(port) - 1, port);
      return send(ok("{}"));
    } else {
      return send(bad_request("Illegal request-target"));
    }
  }

  http::response<http::empty_body> res{http::status::ok, req.version()};
  res.keep_alive(req.keep_alive());
  return send(std::move(res));
}

//char* Session::strstr(const char* string, const char* find)
//{
//  char* char1 = NULL;
//  char* char2 = NULL;
//  if (!string || !find || strlen(string) < strlen(find))
//  {
//    return NULL;
//  }
//
//  for (char1 = (char*)string; (*char1) != '\0'; ++char1)
//  {
//    char* char3 = char1;
//    for (char2 = (char*)find; (*char2) != '\0' && (*char1) != '\0'; ++char2, ++char1)
//    {
//      char c1 = (*char1) & 0xDF;
//      char c2 = (*char2) & 0xDF;
//      if ((c1 != c2) || (((c1 > 0x5A) || (c1 < 0x41)) && (*char1 != *char2)))
//        break;
//    }
//
//    if ((*char2) == '\0')
//      return char3;
//
//    char1 = char3;
//  }
//  return NULL;
//}

int Session::getUrlencodedValue(const char* data, char* item, int maxSize, char* val)
{
  char* p1 = NULL;
  char* p2 = NULL;
  char find[256] = { 0 };

  strcat(find, item);
  strcat(find, "=");
  p1 = (char*)strstr(data, find);
  if (!p1)
    return -1;
  p2 = p1 + strlen(find);
  p1 = p2;

  memset(find, 0, sizeof(find));
  strcpy(find, "&");

  p2 = strstr(p1, find);
  if (!p2)
    p2 = (char*)&data[strlen(data)];

  int valueLen = 0;
  if (p2 - p1 > maxSize)
  {
    return -2;
  }
  memcpy(val, p1, p2 - p1);
  val[p2 - p1] = 0x0;
  valueLen = p2 - p1;
  
  return valueLen;
}

}  // namespace rpc
}  // namespace blockmirror