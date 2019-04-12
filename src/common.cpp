#include <blockmirror/common.h>
#include <boost/date_time/posix_time/posix_time.hpp>

static boost::posix_time::ptime epoch(
    boost::gregorian::date(1970, boost::gregorian::Jan, 1));

namespace blockmirror {

uint64_t now_ms_since_1970() {
  return (boost::posix_time::second_clock::universal_time() - epoch)
      .total_microseconds();
}

}  // namespace blockmirror
