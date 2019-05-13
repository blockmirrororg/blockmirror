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

#include <spdlog/spdlog.h>

#include <blockmirror/config.h>
#include <blockmirror/types.h>

#define ASSERT BOOST_ASSERT
#define VERIFY BOOST_VERIFY

#define B_TRACE spdlog::trace
#define B_DEBUG spdlog::debug
#define B_LOG spdlog::info
#define B_WARN spdlog::warn
#define B_ERR spdlog::error
#define B_CRITICAL spdlog::critical

namespace blockmirror {

uint64_t now_ms_since_1970();

void computeMerkleRoot(std::vector<Hash256> hashes, Hash256 &out);

}  // namespace blockmirror
