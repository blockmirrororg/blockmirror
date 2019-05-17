#pragma once

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/size.hpp>
#include <boost/noncopyable.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/thread/thread.hpp>
#include <boost/variant.hpp>
#include <boost/variant/get.hpp>

#include <spdlog/spdlog.h>

#include <spdlog/fmt/bin_to_hex.h>

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

struct ConfigFile {
  template <typename Archive>
  void serialize(Archive &ar) {
    ar &BOOST_SERIALIZATION_NVP(genesis_pubkey) &
        BOOST_SERIALIZATION_NVP(miner_privkey) &
        BOOST_SERIALIZATION_NVP(reward_pubkey) &
        BOOST_SERIALIZATION_NVP(p2p_bind) & BOOST_SERIALIZATION_NVP(rpc_bind) &
        BOOST_SERIALIZATION_NVP(rpc_resource) & BOOST_SERIALIZATION_NVP(seeds);
  }
  // 创世BP公钥
  Pubkey genesis_pubkey;
  // 挖矿私钥 和 公钥
  Privkey miner_privkey;
  Pubkey miner_pubkey;
  // 奖励公钥
  Pubkey reward_pubkey;
  // 网络绑定端口
  uint16_t p2p_bind;
  uint16_t rpc_bind;
  // RPC资源地址
  std::string rpc_resource;
  // P2P初始连接的节点
  std::vector<std::string> seeds;

  void reset();
  void init(const boost::filesystem::path &path);
};

extern ConfigFile globalConfig;

}  // namespace blockmirror
