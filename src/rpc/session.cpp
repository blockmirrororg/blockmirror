
#include <blockmirror/chain/context.h>
#include <blockmirror/chain/transaction.h>
#include <blockmirror/rpc/session.h>
#include <blockmirror/serialization/ptree_iarchive.h>
#include <blockmirror/store/data_store.h>
#include <blockmirror/store/transaction_store.h>
#include <boost/algorithm/hex.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <blockmirror/serialization/json_oarchive.h>
#include <blockmirror/server.h>

namespace blockmirror {
namespace rpc {

std::map<std::string, Session::GetMethodFuncPtr> Session::_getMethodPtrs;
std::map<std::string, Session::PostMethodFuncPtr> Session::_postMethodPtrs;

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

  handle_request(std::move(req_));
}

void Session::on_write(boost::system::error_code ec,
                       std::size_t bytes_transferred, bool close,
                       bool stopService) {
  boost::ignore_unused(bytes_transferred);

  if (ec) return;

  if (stopService) return _context.getServer().stop();

  if (close) return do_close();

  do_read();
}

void Session::do_close() {
  boost::system::error_code ec;
  socket_.shutdown(tcp::socket::shutdown_send, ec);
}

void Session::handle_request(http::request<http::string_body>&& req) {
  auto const bad_request = [&req](boost::beast::string_view why) {
    http::response<http::string_body> res{http::status::bad_request,
                                          req.version()};
    res.keep_alive(req.keep_alive());
    res.body() = why.to_string();
    res.prepare_payload();
    return res;
  };

  if (req.method() == http::verb::post) {
    deal_post();
  } else if (req.method() == http::verb::get) {
    deal_get();
  } else {
    return lambda_(bad_request("Illegal request-method"));
  }
}

void Session::deal_post()
{
  auto funcPtr = postMethodFuncPtr(req_.target().to_string().c_str());
  (this->*funcPtr)();
}

void Session::deal_get() {

  std::string strTarget = req_.target().to_string();
  const char* target = strTarget.c_str();
  char* ret = (char*)strchr(target, '?');
  if (ret)
  {
	  *ret = 0; // 更改目标、参数分隔符'?'为字符串结尾符'\0'
	  auto funcPtr = getMethodFuncPtr(target);
	  (this->*funcPtr)(ret + 1);
  }
  else
  {
	  auto funcPtr = getMethodFuncPtr(target);
	  (this->*funcPtr)(nullptr);
  }
}

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

void Session::postChainTransaction() {

  http::request<http::string_body>&& req = std::move(req_);
  auto const bad_request = [&req](boost::beast::string_view why) {
    http::response<http::string_body> res{http::status::bad_request,
                                          req.version()};
    res.keep_alive(req.keep_alive());
    res.body() = "{\"error\":\"" + why.to_string() + "\"}";
    res.prepare_payload();
    return res;
  };
  auto const server_error = [&req](boost::beast::string_view what) {
    http::response<http::string_body> res{http::status::internal_server_error,
                                          req.version()};
    res.keep_alive(req.keep_alive());
    res.body() = "{\"error\":\"" + what.to_string() + "\"}";
    res.prepare_payload();
    return res;
  };
  auto const ok = [&req]() {
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.keep_alive(req.keep_alive());
    res.body() = "{}";
    res.prepare_payload();
    return res;
  };

  std::stringstream ss(req_.body());
  boost::property_tree::ptree ptree;
  blockmirror::chain::TransactionSignedPtr transaction =
      std::make_shared<blockmirror::chain::TransactionSigned>();
  try {
    boost::property_tree::read_json(ss, ptree);
    blockmirror::serialization::PTreeIArchive archive(ptree);
    archive >> transaction;
  } catch (std::exception& e) {
    return lambda_(server_error(e.what()));
  }

  if (!_context.check(transaction)) {
    return lambda_(bad_request("check failed"));
  }
  store::TransactionStore& ts = _context.getTransactionStore();
  if (!ts.add(transaction)) {
    return lambda_(bad_request("repeat put, modified!"));
  }

  return lambda_(ok());
}

void Session::postPutData() {

  http::request<http::string_body>&& req = std::move(req_);
  auto const bad_request = [&req](boost::beast::string_view why) {
    http::response<http::string_body> res{http::status::bad_request,
                                          req.version()};
    res.keep_alive(req.keep_alive());
    res.body() = "{\"error\":\"" + why.to_string() + "\"}";
    res.prepare_payload();
    return res;
  };
  auto const server_error = [&req](boost::beast::string_view what) {
    http::response<http::string_body> res{http::status::internal_server_error,
                                          req.version()};
    res.keep_alive(req.keep_alive());
    res.body() = "{\"error\":\"" + what.to_string() + "\"}";
    res.prepare_payload();
    return res;
  };
  auto const ok = [&req]() {
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.keep_alive(req.keep_alive());
    res.body() = "{}";
    res.prepare_payload();
    return res;
  };

  std::stringstream ss(req_.body());
  boost::property_tree::ptree ptree;
  blockmirror::chain::TransactionSignedPtr transaction =
      std::make_shared<blockmirror::chain::TransactionSigned>();
  try {
    boost::property_tree::read_json(ss, ptree);
    blockmirror::serialization::PTreeIArchive archive(ptree);
    archive >> transaction;
  } catch (std::exception& e) {
    return lambda_(server_error(e.what()));
  }

  if (!_context.check(transaction)) {
    return lambda_(bad_request("check failed"));
  }
  store::DataSignatureStore& dss = _context.getDataSignatureStore();
  store::NewDataPtr newData = std::make_shared<chain::scri::NewData>(
      boost::get<chain::scri::NewData>(transaction->getScript()));
  if (!dss.add(newData)) {
    return lambda_(bad_request("repeat put, modified!"));
  }

  return lambda_(ok());
}

void Session::getNodeStop(const char*)
{
	// 鉴权
	std::string str = boost::lexical_cast<std::string>(req_[http::field::api_key]);
	
	http::response<http::string_body> res{ http::status::ok, req_.version() };
	res.keep_alive(req_.keep_alive());
	//res.body() = "{}";
	res.body() = "i received your auth: " + str;
	res.prepare_payload();
	return lambda_(std::move(res), true);
}

void Session::getNodeVersion(const char*)
{
	http::response<http::string_body> res{ http::status::ok, req_.version() };
	res.keep_alive(req_.keep_alive());
	res.body() = "{\"version\":0}";
	res.prepare_payload();
	return lambda_(std::move(res));
}

void Session::getNodePeers(const char*)
{
	http::response<http::string_body> res{ http::status::ok, req_.version() };
	res.keep_alive(req_.keep_alive());
	res.body() = "";
	res.prepare_payload();
	return lambda_(std::move(res));
}

void Session::getNodeConnect(const char* arg) {

	// 需要鉴权
	char host[50];
	char port[50];
	getUrlencodedValue(arg, "port", sizeof(port)-1, port);
	getUrlencodedValue(arg, "host", sizeof(host)-1, host);

	http::response<http::string_body> res{ http::status::ok, req_.version() };
	res.keep_alive(req_.keep_alive());
	res.body() = "{}";
	res.prepare_payload();
	return lambda_(std::move(res));
}

void Session::getChainStatus(const char*) {

  chain::BlockPtr& head = _context.getHead();
  if (head == nullptr) {
    http::response<http::string_body> res{http::status::bad_request,
                                          req_.version()};
    res.keep_alive(req_.keep_alive());
    res.body() = "{\"error\":\"not_found\"}";
    res.prepare_payload();
    return lambda_(std::move(res));
  }

  http::response<http::string_body> res{http::status::ok, req_.version()};
  res.keep_alive(req_.keep_alive());
  res.body() = "{\"height\":\"" +
               boost::lexical_cast<std::string>(head->getHeight()) + "\"}";
  res.prepare_payload();
  return lambda_(std::move(res));
}

void Session::getChainLast(const char*) {

  chain::BlockPtr& head = _context.getHead();
  if (head == nullptr) {
    http::response<http::string_body> res{http::status::bad_request,
                                          req_.version()};
    res.keep_alive(req_.keep_alive());
    res.body() = "{\"error\":\"not_found\"}";
    res.prepare_payload();
    return lambda_(std::move(res));
  }

  std::ostringstream oss;
  blockmirror::serialization::JSONOArchive<std::ostringstream> archive(oss, false);
  archive << head;

  http::response<http::string_body> res{ http::status::ok, req_.version() };
  res.keep_alive(req_.keep_alive());
  res.body() = oss.str();
  res.prepare_payload();
  return lambda_(std::move(res));
}

void Session::getChainBlock(const char* arg) {

  Hash256 key = boost::lexical_cast<Hash256>(arg);
  store::BlockStore& bs = _context.getBlockStore();
  chain::BlockPtr block = bs.getBlock(key);
  if (block == nullptr) {
    http::response<http::string_body> res{http::status::bad_request,
                                          req_.version()};
    res.keep_alive(req_.keep_alive());
    res.body() = "{\"error\":\"not_found\"}";
    res.prepare_payload();
    return lambda_(std::move(res));
  }

  std::ostringstream oss;
  blockmirror::serialization::JSONOArchive<std::ostringstream> archive(oss,
                                                                       false);
  archive << block;

  http::response<http::string_body> res{http::status::ok, req_.version()};
  res.keep_alive(req_.keep_alive());
  res.body() = oss.str();
  res.prepare_payload();
  return lambda_(std::move(res));
}

void Session::getChainTransaction(const char* arg) {

  Hash256Ptr h = std::make_shared<Hash256>(boost::lexical_cast<Hash256>(arg));
  store::TransactionStore& ts = _context.getTransactionStore();
  chain::TransactionSignedPtr t = ts.getTransaction(h);

  if (t == nullptr) {
    http::response<http::string_body> res{http::status::bad_request,
                                          req_.version()};
    res.keep_alive(req_.keep_alive());
    res.body() = "{\"error\":\"not_found\"}";
    res.prepare_payload();
    return lambda_(std::move(res));
  }

  std::ostringstream oss;
  blockmirror::serialization::JSONOArchive<std::ostringstream> archive(oss,
                                                                       false);
  archive << t;

  http::response<http::string_body> res{http::status::ok, req_.version()};
  res.keep_alive(req_.keep_alive());
  res.body() = oss.str();
  res.prepare_payload();
  return lambda_(std::move(res));
}

}  // namespace rpc
}  // namespace blockmirror