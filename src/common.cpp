#include <blockmirror/common.h>
#include <blockmirror/crypto/ecc.h>
#include <blockmirror/serialization/ptree_iarchive.h>
#include <openssl/sha.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/property_tree/json_parser.hpp>
#define BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED
#include <boost/stacktrace.hpp>

static boost::posix_time::ptime epoch(
    boost::gregorian::date(1970, boost::gregorian::Jan, 1));

namespace boost {
void assertion_failed(char const *expr, char const *function,
                      char const *stream, long line) {
  std::cerr << boost::stacktrace::stacktrace() << std::endl;
  B_CRITICAL("{} at {}({})", expr, stream, line);
  throw std::runtime_error("assertion_failed");
}
void assertion_failed_msg(char const *expr, char const *msg,
                          char const *function, char const *stream, long line) {
  std::cerr << boost::stacktrace::stacktrace() << std::endl;
  B_CRITICAL("{} {} at {}({})", msg, expr, stream, line);
  throw std::runtime_error("assertion_failed_msg");
}
}  // namespace boost

namespace blockmirror {

uint64_t now_ms_since_1970() {
  return (boost::posix_time::microsec_clock::universal_time() - epoch)
             .total_microseconds() /
         1000;
}

void computeMerkleRoot(std::vector<Hash256> hashes, Hash256 &out) {
  if (hashes.size() == 0) {
    out.fill(0);
    return;
  }
  SHA256_CTX ctx;
  while (hashes.size() > 1) {
    if ((hashes.size() % 2) != 0) {
      hashes.push_back(hashes.back());
    }
    for (size_t i = 0; i < hashes.size() / 2; i++) {
      SHA256_Init(&ctx);
      SHA256_Update(&ctx, hashes[2 * i].data(), hashes[2 * i].size() * 2);
      SHA256_Final(hashes[i].data(), &ctx);
    }
    hashes.resize(hashes.size() / 2);
  }
  out = hashes.back();
}

ConfigFile globalConfig;
void ConfigFile::reset() {
  genesis_pubkey.fill(0);
  miner_privkey.fill(0);
  miner_pubkey.fill(0);
  reward_pubkey.fill(0);
  p2p_bind = 0;
  rpc_bind = 0;
}
void ConfigFile::init(const boost::filesystem::path &path) {
  reset();
  std::ifstream stream;
  stream.open(path.string(), std::ios_base::in);
  stream.exceptions(std::fstream::failbit | std::fstream::badbit |
                    std::fstream::eofbit);
  boost::property_tree::ptree ptree;
  boost::property_tree::read_json(stream, ptree);
  serialization::PTreeIArchive archive(ptree);
  archive >> *this;

  if (genesis_pubkey == Pubkey({0})) {
    throw std::runtime_error("config genesis_pubkey not found");
  }
  B_LOG("genesis {:spn}", spdlog::to_hex(genesis_pubkey));
  if (miner_privkey != Privkey({0})) {
    crypto::ECC.computePub(miner_privkey, miner_pubkey);
  }
  B_LOG("miner {:spn}", spdlog::to_hex(miner_pubkey));
  if (reward_pubkey == Pubkey{0}) {
    reward_pubkey = miner_pubkey;
  }
  B_LOG("reward {:spn}", spdlog::to_hex(reward_pubkey));
  if (p2p_bind == 0) {
    throw std::runtime_error("config p2p_bind not found");
  }
  if (rpc_bind == 0) {
    throw std::runtime_error("config rpc_bind not found");
  }
  B_LOG("rpc port {} p2p port {}", rpc_bind, p2p_bind);
  if (!boost::filesystem::exists(rpc_resource)) {
    throw std::runtime_error("config rpc_resource not found");
  }
}

}  // namespace blockmirror
