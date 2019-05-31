
#include <blockmirror/chain/context.h>
#include <blockmirror/chain/transaction.h>
#include <blockmirror/rpc/session.h>
#include <blockmirror/serialization/binary_oarchive.h>
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

Session::Session(tcp::socket socket, blockmirror::chain::Context& context,
                 boost::asio::io_context& ioc)
    : socket_(std::move(socket)),
#if BOOST_VERSION >= 107000
      strand_(boost::asio::make_strand(ioc)),
#else
      strand_(socket_.get_executor()),
#endif

      lambda_(*this),
      _context(context) {
}

void Session::run() { do_read(); }

void Session::do_read() {
  req_ = {};
  // stream_.expires_after(std::chrono::seconds(30));

  /*http::async_read(
      stream_, buffer_, req_,
      boost::beast::bind_front_handler(&Session::on_read,
     shared_from_this()));*/
  http::async_read(
      socket_, buffer_, req_,
      boost::asio::bind_executor(
          strand_, std::bind(&Session::on_read, shared_from_this(),
                             std::placeholders::_1, std::placeholders::_2)));
}

void Session::on_read(boost::system::error_code ec,
                      std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);

  if (ec) {
    B_WARN("http on read {} {}", ec.message(), req_.body());
  }

  if (ec == http::error::end_of_stream) return do_close();

  if (ec) return;

  handle_request();
}

// void Session::on_write(bool close, beast::error_code ec,
//                       std::size_t bytes_transferred, bool stopService) {
void Session::on_write(boost::system::error_code ec,
                       std::size_t bytes_transferred, bool close,
                       bool stopService) {
  boost::ignore_unused(bytes_transferred);

  if (ec) {
    B_WARN("http on write {}", ec.message());
    return;
  }

  if (stopService) {
    blockmirror::Server::get().stop();
    return;
  }

  if (close) {
    do_close();
    return;
  }

  res_ = nullptr;

  do_read();
}

void Session::do_close() {
  B_LOG("close connection {}", socket_.remote_endpoint().address().to_string());

  boost::system::error_code ec;
  socket_.shutdown(tcp::socket::shutdown_send, ec);
  // stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
}

void Session::handle_request() {
  B_LOG("handle http request {} \n {}", req_.target().to_string(), req_.body());

  if (req_.method() == http::verb::post) {
    auto funcPtr = postMethodFuncPtr(req_.target().to_string().c_str());
    if (!funcPtr) {
      return lambda_(bad_request("Illegal request-target"));
    }
    (this->*funcPtr)();

  } else if (req_.method() == http::verb::get) {
    /*if (req_.target().find(".") != std::string::npos) {
      handle_file(std::move(req_));
    } else {*/
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
      std::string out;
      if (!url_decode(ret + 1, out)) {
        B_WARN("url_decode failed");
        return lambda_(bad_request("url_decode failed"));
      }
      //(this->*funcPtr)(ret + 1);
      (this->*funcPtr)(out.c_str());
    } else {
      (this->*funcPtr)(nullptr);
    }
    /*}*/

  } else {
    return lambda_(bad_request("Illegal request-method"));
  }
}

int Session::getUrlencodedValue(const char* data, const char* item, int maxSize,
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
  std::stringstream ss(req_.body());
  boost::property_tree::ptree ptree;
  blockmirror::chain::TransactionSignedPtr transaction =
      std::make_shared<blockmirror::chain::TransactionSigned>();
  try {
    boost::property_tree::read_json(ss, ptree);
    blockmirror::serialization::PTreeIArchive archive(ptree);
    archive >> transaction;
  } catch (std::exception& e) {
    B_WARN("{}", e.what());
    return lambda_(server_error(e.what()));
  } catch (...) {
    B_WARN("unknown exception!");
    return lambda_(server_error("unknown exception!"));
  }

  if (!_context.check(transaction)) {
    B_WARN("transaction check failed");
    return lambda_(bad_request("check failed"));
  }
  store::TransactionStore& ts = _context.getTransactionStore();
  if (!ts.add(transaction)) {
    B_WARN("repeat put, modified!");
    return lambda_(bad_request("repeat put, modified!"));
  }

  return lambda_(ok("{}"));
}

void Session::postChainData() {
  std::stringstream ss(req_.body());
  boost::property_tree::ptree ptree;
  chain::DataPtr data = std::make_shared<chain::Data>();
  try {
    boost::property_tree::read_json(ss, ptree);
    blockmirror::serialization::PTreeIArchive archive(ptree);
    archive >> data;
  } catch (std::exception& e) {
    B_WARN("{}", e.what());
    return lambda_(server_error(e.what()));
  } catch (...) {
    B_WARN("unknown exception!");
    return lambda_(server_error("unknown exception!"));
  }

  if (!_context.check(data)) {
    B_WARN("data check failed");
    return lambda_(bad_request("check failed"));
  }

  chain::DataSignedPtr dataSigned =
      std::make_shared<chain::DataSigned>(data->getName(), data->getData());
  try {
    dataSigned->sign(globalConfig.miner_privkey,
                     _context.getHead()->getHeight());
  } catch (std::exception& e) {
    B_WARN("{}", e.what());
    return lambda_(server_error(e.what()));
  } catch (...) {
    B_WARN("unknown exception!");
    return lambda_(server_error("unknown exception!"));
  }

  if (!_context.getDataSignatureStore().add(dataSigned)) {
    B_WARN("repeat put, modified!");
    return lambda_(bad_request("repeat put, modified!"));
  }

  return lambda_(ok("{}"));
}

void Session::getNodeStop(const char*) {
  // 鉴权
  std::string str =
      boost::lexical_cast<std::string>(req_[http::field::authorization]);
  return lambda_(ok("{}"), true);
}

void Session::getNodeVersion(const char*) {
  return lambda_(ok("{\"version\":0}"));
}

void Session::getNodePeers(const char*) { return lambda_(ok("{}")); }

void Session::getNodeConnect(const char* arg) {
  if (!arg) {
    B_WARN("omit argument");
    return lambda_(bad_request("omit argument"));
  }

  // 需要鉴权
  char host[50] = {0};
  char port[50] = {0};
  getUrlencodedValue(arg, "port", sizeof(port) - 1, port);
  getUrlencodedValue(arg, "host", sizeof(host) - 1, host);

  return lambda_(ok("{}"));
}

void Session::getChainStatus(const char*) {
  chain::BlockPtr& head = _context.getHead();
  if (head == nullptr) {
    B_WARN("head not found");
    return lambda_(bad_request("not found"));
  }

  return lambda_(ok("{\"height\":\"" +
                    boost::lexical_cast<std::string>(head->getHeight()) +
                    "\"}"));
}

void Session::getChainLast(const char*) {
  chain::BlockPtr& head = _context.getHead();
  if (head == nullptr) {
    B_WARN("head not found");
    return lambda_(bad_request("not found"));
  }

  std::ostringstream oss;
  blockmirror::serialization::JSONOArchive<std::ostringstream> archive(oss,
                                                                       false);
  try {
    archive << head;
  } catch (std::exception& e) {
    B_WARN("{}", e.what());
    return lambda_(server_error(e.what()));
  } catch (...) {
    B_WARN("unknown exception!");
    return lambda_(server_error("unknown exception!"));
  }

  return lambda_(ok(oss.str()));
}

void Session::getChainBlock(const char* arg) {
  if (!arg) {
    B_WARN("omit argument");
    return lambda_(bad_request("omit argument"));
  }

  Hash256 key;
  try {
    boost::algorithm::unhex(arg, key.begin());
  } catch (std::exception& e) {
    B_WARN("{}", e.what());
    return lambda_(server_error(e.what()));
  } catch (...) {
    B_WARN("unknown exception!");
    return lambda_(server_error("unknown exception!"));
  }

  store::BlockStore& bs = _context.getBlockStore();
  chain::BlockPtr block = bs.getBlock(key);
  if (block == nullptr) {
    return lambda_(ok("{}"));
  }

  std::ostringstream oss;
  blockmirror::serialization::JSONOArchive<std::ostringstream> archive(oss,
                                                                       false);
  try {
    archive << block;
  } catch (std::exception& e) {
    B_WARN("{}", e.what());
    return lambda_(server_error(e.what()));
  } catch (...) {
    B_WARN("unknown exception!");
    return lambda_(server_error("unknown exception!"));
  }

  return lambda_(ok(oss.str()));
}

void Session::getChainTransaction(const char* arg) {
  if (!arg) {
    B_WARN("omit argument");
    return lambda_(bad_request("omit argument"));
  }

  Hash256Ptr key;
  try {
    boost::algorithm::unhex(arg, key->begin());
  } catch (std::exception& e) {
    B_WARN("{}", e.what());
    return lambda_(server_error(e.what()));
  } catch (...) {
    B_WARN("unknown exception!");
    return lambda_(server_error("unknown exception!"));
  }

  store::TransactionStore& ts = _context.getTransactionStore();
  chain::TransactionSignedPtr t = ts.getTransaction(key);

  if (t == nullptr) {
    return lambda_(ok("{}"));
  }

  std::ostringstream oss;
  blockmirror::serialization::JSONOArchive<std::ostringstream> archive(oss,
                                                                       false);
  try {
    archive << t;
  } catch (std::exception& e) {
    B_WARN("{}", e.what());
    return lambda_(server_error(e.what()));
  } catch (...) {
    B_WARN("unknown exception!");
    return lambda_(server_error("unknown exception!"));
  }

  return lambda_(ok(oss.str()));
}

void Session::getChainFormat(const char* arg) {
  if (!arg) {
    B_WARN("omit argument");
    return lambda_(bad_request("omit argument"));
  }

  store::FormatStore& fs = _context.getFormatStore();
  store::NewFormatPtr format = fs.query(arg);
  if (!format) {
    return lambda_(ok("{}"));
  }

  std::ostringstream oss;
  blockmirror::serialization::JSONOArchive<std::ostringstream> archive(oss,
                                                                       false);
  try {
    archive << format;
  } catch (std::exception& e) {
    B_WARN("{}", e.what());
    return lambda_(server_error(e.what()));
  } catch (...) {
    B_WARN("unknown exception!");
    return lambda_(server_error("unknown exception!"));
  }

  return lambda_(ok(oss.str()));
}

void Session::getChainFormats(const char*) {
  store::FormatStore& fs = _context.getFormatStore();

  std::ostringstream oss;
  blockmirror::serialization::JSONOArchive<std::ostringstream> archive(oss,
                                                                       false);
  try {
    archive << fs;
  } catch (std::exception& e) {
    B_WARN("{}", e.what());
    return lambda_(server_error(e.what()));
  } catch (...) {
    B_WARN("unknown exception!");
    return lambda_(server_error("unknown exception!"));
  }

  return lambda_(ok(oss.str()));
}

void Session::getChainDatatypes(const char* arg) {
  if (!arg) {
    B_WARN("omit argument");
    return lambda_(bad_request("omit argument"));
  }

  store::DataStore& ds = _context.getDataStore();
  store::NewDataPtr data = ds.query(arg);
  if (!data) {
    return lambda_(ok("{}"));
  }

  std::ostringstream oss;
  blockmirror::serialization::JSONOArchive<std::ostringstream> archive(oss,
                                                                       false);
  try {
    archive << data;
  } catch (std::exception& e) {
    B_WARN("{}", e.what());
    return lambda_(server_error(e.what()));
  } catch (...) {
    B_WARN("unknown exception!");
    return lambda_(server_error("unknown exception!"));
  }

  return lambda_(ok(oss.str()));
}

void Session::getChainDatatype(const char* arg) {
  if (!arg) {
    B_WARN("omit argument");
    return lambda_(bad_request("omit argument"));
  }

  store::DataStore& ds = _context.getDataStore();
  std::vector<store::NewDataPtr> v = ds.queryFormat(arg);

  std::ostringstream oss;
  blockmirror::serialization::JSONOArchive<std::ostringstream> archive(oss,
                                                                       false);
  try {
    archive << v;
  } catch (std::exception& e) {
    B_WARN("{}", e.what());
    return lambda_(server_error(e.what()));
  } catch (...) {
    B_WARN("unknown exception!");
    return lambda_(server_error("unknown exception!"));
  }

  return lambda_(ok(oss.str()));
}

void Session::getChainBps(const char*) {
  std::vector<Pubkey>& bps = _context.getBpsStore().getBps();

  std::ostringstream oss;
  blockmirror::serialization::JSONOArchive<std::ostringstream> archive(oss,
                                                                       false);
  try {
    archive << bps;
  } catch (std::exception& e) {
    B_WARN("{}", e.what());
    return lambda_(server_error(e.what()));
  } catch (...) {
    B_WARN("unknown exception!");
    return lambda_(server_error("unknown exception!"));
  }

  return lambda_(ok(oss.str()));
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

Session::GetMethodFuncPtr Session::getMethodFuncPtr(const char* target) {
  auto pos = _getMethodPtrs.find(target);
  if (pos != _getMethodPtrs.end()) {
    return pos->second;
  } else {
    return nullptr;
  }
}

Session::PostMethodFuncPtr Session::postMethodFuncPtr(const char* target) {
  auto pos = _postMethodPtrs.find(target);
  if (pos != _postMethodPtrs.end()) {
    return pos->second;
  } else {
    return nullptr;
  }
}

http::response<http::string_body> Session::bad_request(std::string why) {
  http::response<http::string_body> res{http::status::bad_request,
                                        req_.version()};
  res.keep_alive(req_.keep_alive());
  res.body() = "{\"error\":\"" + why + "\"}";
  res.set(http::field::content_type, "application/json");
  res.prepare_payload();
  return res;
}

http::response<http::string_body> Session::server_error(std::string what) {
  http::response<http::string_body> res{http::status::internal_server_error,
                                        req_.version()};
  res.keep_alive(req_.keep_alive());
  res.body() = "{\"error\":\"" + what + "\"}";
  res.set(http::field::content_type, "application/json");
  res.prepare_payload();
  return res;
}

http::response<http::string_body> Session::ok(std::string what) {
  http::response<http::string_body> res{http::status::ok, req_.version()};
  res.keep_alive(req_.keep_alive());
  res.body() = what;
  res.set(http::field::content_type, "application/json");
  res.prepare_payload();
  return res;
}

bool Session::url_decode(const std::string& in, std::string& out) {
  out.clear();
  out.reserve(in.size());
  for (std::size_t i = 0; i < in.size(); ++i) {
    if (in[i] == '%') {
      if (i + 3 <= in.size()) {
        int value = 0;
        std::istringstream is(in.substr(i + 1, 2));
        if (is >> std::hex >> value) {
          out += static_cast<char>(value);
          i += 2;
        } else {
          return false;
        }
      } else {
        return false;
      }
    } else if (in[i] == '+') {
      out += ' ';
    } else {
      out += in[i];
    }
  }
  return true;
}

}  // namespace rpc
}  // namespace blockmirror