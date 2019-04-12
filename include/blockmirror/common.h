#pragma once

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <type_traits>
#include <vector>

#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/variant.hpp>

#include <blockmirror/types.h>

#define ASSERT BOOST_ASSERT
#define VERIFY BOOST_VERIFY
#define LOG printf
#define WARN printf
#define ERR printf

namespace blockmirror {

uint64_t now_ms_since_1970();

}  // namespace blockmirror
