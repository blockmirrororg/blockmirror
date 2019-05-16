#include <blockmirror/common.h>
#include <openssl/sha.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#define BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED
#include <boost/stacktrace.hpp>

static boost::posix_time::ptime epoch(
    boost::gregorian::date(1970, boost::gregorian::Jan, 1));

namespace boost {
void assertion_failed(char const *expr, char const *function, char const *file,
                      long line) {
  std::cerr << boost::stacktrace::stacktrace() << std::endl;
  B_CRITICAL("{} at {}({})", expr, file, line);
  throw std::runtime_error("assertion_failed");
}
void assertion_failed_msg(char const *expr, char const *msg,
                          char const *function, char const *file, long line) {
  std::cerr << boost::stacktrace::stacktrace() << std::endl;
  B_CRITICAL("{} {} at {}({})", msg, expr, file, line);
  throw std::runtime_error("assertion_failed_msg");
}
}  // namespace boost

namespace blockmirror {

uint64_t now_ms_since_1970() {
  return (boost::posix_time::second_clock::universal_time() - epoch)
      .total_microseconds();
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

}  // namespace blockmirror
