
#include <blockmirror/chain/context.h>
#include <blockmirror/chain/transaction.h>
#include <blockmirror/rpc/session.h>
#include <blockmirror/serialization/json_oarchive.h>
#include <blockmirror/serialization/ptree_iarchive.h>
#include <blockmirror/server.h>
#include <blockmirror/store/data_store.h>
#include <blockmirror/store/transaction_store.h>
#include <boost/algorithm/hex.hpp>
#include <boost/property_tree/json_parser.hpp>

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

  if (stopService) {
    blockmirror::Server::get().stop();
    return;
  }

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
    auto funcPtr = postMethodFuncPtr(req_.target().to_string().c_str());
    if (!funcPtr) {
      return lambda_(bad_request("Illegal request-target"));
    }
    (this->*funcPtr)();

  } else if (req.method() == http::verb::get) {
    if (req_.target().find(".") != std::string::npos) {
      handle_file(std::move(req_));
    } else {
      std::string strTarget = req_.target().to_string();
      const char* target = strTarget.c_str();
      char* ret = (char*)strchr(target, '?');
      if (ret) {
        *ret = 0;  // 更改目标、参数分隔符'?'为字符串结尾符'\0'
      }
      auto funcPtr = getMethodFuncPtr(target);
      if (!funcPtr) {
        return lambda_(bad_request("Illegal request-target"));
      }

      if (ret) {
        (this->*funcPtr)(ret + 1);
      } else {
        (this->*funcPtr)(nullptr);
      }
    }

  } else {
    return lambda_(bad_request("Illegal request-method"));
  }
}

int Session::getUrlencodedValue(const char* data, char* item, int maxSize,
                                char* val) {
  char* p1 = NULL;
  char* p2 = NULL;
  char find[256] = {0};

  strcat(find, item);
  strcat(find, "=");
  p1 = (char*)strstr(data, find);
  if (!p1) return -1;
  p2 = p1 + strlen(find);
  p1 = p2;

  memset(find, 0, sizeof(find));
  strcpy(find, "&");

  p2 = strstr(p1, find);
  if (!p2) p2 = (char*)&data[strlen(data)];

  int valueLen = 0;
  if (p2 - p1 > maxSize) {
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

//void Session::postPutData() {
//  http::request<http::string_body>&& req = std::move(req_);
//  auto const bad_request = [&req](boost::beast::string_view why) {
//    http::response<http::string_body> res{http::status::bad_request,
//                                          req.version()};
//    res.keep_alive(req.keep_alive());
//    res.body() = "{\"error\":\"" + why.to_string() + "\"}";
//    res.prepare_payload();
//    return res;
//  };
//  auto const server_error = [&req](boost::beast::string_view what) {
//    http::response<http::string_body> res{http::status::internal_server_error,
//                                          req.version()};
//    res.keep_alive(req.keep_alive());
//    res.body() = "{\"error\":\"" + what.to_string() + "\"}";
//    res.prepare_payload();
//    return res;
//  };
//  auto const ok = [&req]() {
//    http::response<http::string_body> res{http::status::ok, req.version()};
//    res.keep_alive(req.keep_alive());
//    res.body() = "{}";
//    res.prepare_payload();
//    return res;
//  };
//
//  std::stringstream ss(req_.body());
//  boost::property_tree::ptree ptree;
//  blockmirror::chain::TransactionSignedPtr transaction =
//      std::make_shared<blockmirror::chain::TransactionSigned>();
//  try {
//    boost::property_tree::read_json(ss, ptree);
//    blockmirror::serialization::PTreeIArchive archive(ptree);
//    archive >> transaction;
//  } catch (std::exception& e) {
//    return lambda_(server_error(e.what()));
//  }
//
//  if (!_context.check(transaction)) {
//    return lambda_(bad_request("check failed"));
//  }
//  store::DataSignatureStore& dss = _context.getDataSignatureStore();
//  store::NewDataPtr newData = std::make_shared<chain::scri::NewData>(
//      boost::get<chain::scri::NewData>(transaction->getScript()));
//  if (!dss.add(newData)) {
//    return lambda_(bad_request("repeat put, modified!"));
//  }
//
//  return lambda_(ok());
//}

void Session::postChainData() {
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
  chain::DataPtr data = std::make_shared<chain::Data>();
  try {
    boost::property_tree::read_json(ss, ptree);
    blockmirror::serialization::PTreeIArchive archive(ptree);
    archive >> data;
  } catch (std::exception& e) {
    return lambda_(server_error(e.what()));
  }

  store::NewDataPtr newData = _context.getDataStore().query(data->getName());
  if (!newData) {
    return lambda_(bad_request("data type not found"));
  }
  store::NewFormatPtr newFormat =
      _context.getFormatStore().query(newData->getFormat());
  if (!newFormat) {
    return lambda_(bad_request("data format not found"));
  }

  return lambda_(ok());
}

void Session::getNodeStop(const char*)
{
	// 鉴权
	std::string str = boost::lexical_cast<std::string>(req_[http::field::authorization]);
	
	http::response<http::string_body> res{ http::status::ok, req_.version() };
	res.keep_alive(req_.keep_alive());
	//res.body() = "{}";
	res.body() = "i received your authorization: " + str;
	res.prepare_payload();
	return lambda_(std::move(res), true);
}

void Session::getNodeVersion(const char*) {
  http::response<http::string_body> res{http::status::ok, req_.version()};
  res.keep_alive(req_.keep_alive());
  res.body() = "{\"version\":0}";
  res.prepare_payload();
  return lambda_(std::move(res));
}

void Session::getNodePeers(const char*) {
  http::response<http::string_body> res{http::status::ok, req_.version()};
  res.keep_alive(req_.keep_alive());
  res.body() = "";
  res.prepare_payload();
  return lambda_(std::move(res));
}

void Session::getNodeConnect(const char* arg) {

  if (!arg) {
    http::response<http::string_body> res{http::status::bad_request,
                                          req_.version()};
    res.keep_alive(req_.keep_alive());
    res.body() = "{\"error\":\"omit argument\"}";
    res.prepare_payload();
    return lambda_(std::move(res));
  }

  // 需要鉴权
  char host[50] = { 0 };
  char port[50] = { 0 };
  getUrlencodedValue(arg, "port", sizeof(port) - 1, port);
  getUrlencodedValue(arg, "host", sizeof(host) - 1, host);

  http::response<http::string_body> res{http::status::ok, req_.version()};
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
  blockmirror::serialization::JSONOArchive<std::ostringstream> archive(oss,
                                                                       false);
  archive << head;

  http::response<http::string_body> res{http::status::ok, req_.version()};
  res.keep_alive(req_.keep_alive());
  res.body() = oss.str();
  res.prepare_payload();
  return lambda_(std::move(res));
}

void Session::getChainBlock(const char* arg) {

  if (!arg) {
    http::response<http::string_body> res{http::status::bad_request,
                                          req_.version()};
    res.keep_alive(req_.keep_alive());
    res.body() = "{\"error\":\"omit argument\"}";
    res.prepare_payload();
    return lambda_(std::move(res));
  }

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

  if (!arg) {
    http::response<http::string_body> res{http::status::bad_request,
                                          req_.version()};
    res.keep_alive(req_.keep_alive());
    res.body() = "{\"error\":\"omit argument\"}";
    res.prepare_payload();
    return lambda_(std::move(res));
  }

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

void Session::handle_file(http::request<http::string_body>&& req) {
  // Returns a not found response
  auto const not_found = [&req](beast::string_view target) {
    http::response<http::string_body> res{http::status::not_found,
                                          req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "The resource '" + std::string(target) + "' was not found.";
    res.prepare_payload();
    return res;
  };

  // Returns a server error response
  auto const server_error = [&req](beast::string_view what) {
    http::response<http::string_body> res{http::status::internal_server_error,
                                          req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "An error occurred: '" + std::string(what) + "'";
    res.prepare_payload();
    return res;
  };

  // Build the path to the requested file
  std::string path = path_cat(doc_root, req.target());
  if (req.target().back() == '/') path.append("index.html");

  // Attempt to open the file
  beast::error_code ec;
  http::file_body::value_type body;
  body.open(path.c_str(), beast::file_mode::scan, ec);

  // Handle the case where the file doesn't exist
  if (ec == beast::errc::no_such_file_or_directory)
    return lambda_(not_found(req.target()));

  // Handle an unknown error
  if (ec) return lambda_(server_error(ec.message()));

  // Cache the size since we need it after the move
  auto const size = body.size();

  // Respond to HEAD request
  if (req.method() == http::verb::head) {
    http::response<http::empty_body> res{http::status::ok, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return lambda_(std::move(res));
  }

  // Respond to GET request
  http::response<http::file_body> res{
      std::piecewise_construct, std::make_tuple(std::move(body)),
      std::make_tuple(http::status::ok, req.version())};
  res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
  res.set(http::field::content_type, mime_type(path));
  res.content_length(size);
  res.keep_alive(req.keep_alive());
  return lambda_(std::move(res));
}

beast::string_view Session::mime_type(beast::string_view path) {
  using beast::iequals;
  auto const ext = [&path] {
    auto const pos = path.rfind(".");
    if (pos == beast::string_view::npos) return beast::string_view{};
    return path.substr(pos);
  }();
  if (iequals(ext, ".htm")) return "text/html";
  if (iequals(ext, ".html")) return "text/html";
  if (iequals(ext, ".php")) return "text/html";
  if (iequals(ext, ".css")) return "text/css";
  if (iequals(ext, ".txt")) return "text/plain";
  if (iequals(ext, ".js")) return "application/javascript";
  if (iequals(ext, ".json")) return "application/json";
  if (iequals(ext, ".xml")) return "application/xml";
  if (iequals(ext, ".swf")) return "application/x-shockwave-flash";
  if (iequals(ext, ".flv")) return "video/x-flv";
  if (iequals(ext, ".png")) return "image/png";
  if (iequals(ext, ".jpe")) return "image/jpeg";
  if (iequals(ext, ".jpeg")) return "image/jpeg";
  if (iequals(ext, ".jpg")) return "image/jpeg";
  if (iequals(ext, ".gif")) return "image/gif";
  if (iequals(ext, ".bmp")) return "image/bmp";
  if (iequals(ext, ".ico")) return "image/vnd.microsoft.icon";
  if (iequals(ext, ".tiff")) return "image/tiff";
  if (iequals(ext, ".tif")) return "image/tiff";
  if (iequals(ext, ".svg")) return "image/svg+xml";
  if (iequals(ext, ".svgz")) return "image/svg+xml";
  return "application/text";
}

std::string Session::path_cat(beast::string_view base,
                              beast::string_view path) {
  if (base.empty()) return std::string(path);
  std::string result(base);
  char constexpr path_separator = '/';
  if (result.back() == path_separator) result.resize(result.size() - 1);
  result.append(path.data(), path.size());
  return result;
}

}  // namespace rpc
}  // namespace blockmirror