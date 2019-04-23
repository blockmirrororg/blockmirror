#pragma once

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <set>

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/size.hpp>
#include <boost/noncopyable.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/variant.hpp>
#include <boost/variant/get.hpp>
#include <boost/thread/thread.hpp>
#include <boost/lexical_cast.hpp>

#include <blockmirror/config.h>
#include <blockmirror/types.h>

#define ASSERT BOOST_ASSERT
#define VERIFY BOOST_VERIFY
#define LOG printf
#define WARN printf
#define ERR printf

namespace blockmirror {

uint64_t now_ms_since_1970();

void computeMerkleRoot(std::vector<Hash256> hashes, Hash256 &out);

}  // namespace blockmirror
