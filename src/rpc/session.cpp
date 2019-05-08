
#include <blockmirror/chain/transaction.h>
#include <blockmirror/rpc/session.h>
#include <blockmirror/serialization/ptree_iarchive.h>
#include <blockmirror/store/data_store.h>
#include <blockmirror/store/transaction_store.h>
#include <boost/algorithm/hex.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace blockmirror {
namespace rpc {

std::string bp1Pub =
    "02ECCAE0C5766164670E17C7F6796294375BE8CD3F3F135C035CE8C3024D54B6D4";

blockmirror::Pubkey P(const std::string& str) {
  blockmirror::Pubkey pub;
  boost::algorithm::unhex(str, pub.begin());
  return pub;
}

Session::Session(tcp::socket socket,
                 std::shared_ptr<std::string const> const& doc_root)
    : socket_(std::move(socket)),
      strand_(socket_.get_executor()),
      doc_root_(doc_root),
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

boost::beast::string_view mime_type(boost::beast::string_view path) {
  using boost::beast::iequals;
  auto const ext = [&path] {
    auto const pos = path.rfind(".");
    if (pos == boost::beast::string_view::npos)
      return boost::beast::string_view{};
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

std::string path_cat(boost::beast::string_view base,
                     boost::beast::string_view path) {
  if (base.empty()) return path.to_string();
  std::string result = base.to_string();
#if BOOST_MSVC
  char constexpr path_separator = '\\';
  if (result.back() == path_separator) result.resize(result.size() - 1);
  result.append(path.data(), path.size());
  for (auto& c : result)
    if (c == '/') c = path_separator;
#else
  char constexpr path_separator = '/';
  if (result.back() == path_separator) result.resize(result.size() - 1);
  result.append(path.data(), path.size());
#endif
  return result;
}

template <typename T>
void IJ(const std::string& value, T& out) {
  std::stringstream ss(value);
  boost::property_tree::ptree ptree;
  boost::property_tree::read_json(ss, ptree);
  blockmirror::serialization::PTreeIArchive archive(ptree);
  archive >> out;
}

void Session::on_read(boost::system::error_code ec,
                      std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);

  if (ec == http::error::end_of_stream) return do_close();

  if (ec) return;

  handle_request(*doc_root_, std::move(req_), lambda_);
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
    boost::beast::string_view doc_root,
    http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
  // Returns a bad request response
  auto const bad_request = [&req](boost::beast::string_view why) {
    http::response<http::string_body> res{http::status::bad_request,
                                          req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = why.to_string();
    res.prepare_payload();
    return res;
  };

  if (req.method() != http::verb::put)  // put transaction and put data
    return send(bad_request("Unknown HTTP-method"));

  if (req.target() == "transaction") {
    blockmirror::chain::TransactionSignedPtr transaction =
        std::make_shared<blockmirror::chain::TransactionSigned>();
    std::string strTransaction = req.body();  // json format
    IJ(strTransaction, transaction);
    transaction->setExpire(2);
    transaction->setNonce();
    transaction->verify();

    blockmirror::store::TransactionStore ts;
    boost::filesystem::path p("/ze");
    ts.load(p);
    ts.add(transaction);
  } else if (req.target() == "data") {
    blockmirror::chain::DataSignedPtr data =
        std::make_shared<blockmirror::chain::DataSigned>();
    std::string strData = req.body();  // json format
    IJ(strData, data);
    uint64_t height = 100;
    data->verify(P(bp1Pub), height);

    blockmirror::store::DataStore ds;
    boost::filesystem::path p("/ze");
    ds.load(p);
    // ds.add(data);
  }

  if (req.keep_alive()) {
    do_read();
  } else {
    do_close();
  }
}

}  // namespace rpc
}  // namespace blockmirror