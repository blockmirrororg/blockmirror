#pragma once

#include <fstream>
#include <iostream>

#include <boost/algorithm/hex.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <blockmirror/chain/block.h>
#include <blockmirror/common.h>
#include <blockmirror/serialization/binary_iarchive.h>
#include <blockmirror/serialization/binary_oarchive.h>
#include <blockmirror/serialization/json_oarchive.h>
#include <blockmirror/serialization/ptree_iarchive.h>

// 从一个空构造的BLOCK 尽量包含所有元素
void initFullBlock(blockmirror::chain::Block &blk);

// 删除所有的 Context 文件
void removeContextFiles();

// 字符串转公钥私钥
blockmirror::Privkey PRIV(const std::string &str);
blockmirror::Pubkey PUB(const std::string &str);

// 可序列化类型 <=> JSON字符串
template <typename T>
static inline std::string SerOToJson(const T &obj, bool indent = true) {
  std::ostringstream oss;
  blockmirror::serialization::JSONOArchive<std::ostringstream> archive(oss,
                                                                       indent);
  archive << obj;
  return oss.str();
}
template <typename T>
static inline void SerOFromJSON(const std::string &str, T &out) {
  std::stringstream ss(str);
  boost::property_tree::ptree ptree;
  boost::property_tree::read_json(ss, ptree);
  blockmirror::serialization::PTreeIArchive archive(ptree);
  archive >> out;
}
// 可序列化类型 <=> BINARY(std::string是存储手段)
template <typename T>
static inline std::string SerOToBin(const T &obj) {
  std::ostringstream oss;
  blockmirror::serialization::BinaryOArchive<std::ostringstream> archive(oss);
  archive << obj;
  return oss.str();
}
template <typename T>
static inline void SerOFromBin(const std::string &value, T &out) {
  std::istringstream iss(value);
  blockmirror::serialization::BinaryIArchive<std::istringstream> archive(iss);
  archive >> out;
}
// 可序列化类型 <=> HEX字符串
template <typename T>
static inline std::string SerOToHex(const T &obj) {
  std::string str = SerOToBin<T>(obj);
  return boost::algorithm::hex(str);
}
template <typename T>
static inline void SerOFromHex(const std::string &hex, T &out) {
  std::string value = boost::algorithm::unhex(hex);
  std::istringstream iss(value);
  blockmirror::serialization::BinaryIArchive<std::istringstream> archive(iss);
  archive >> out;
}

// 张三
extern std::string pubZhangsan;
extern std::string privZhangsan;

// 李四
extern std::string pubLisi;
extern std::string privLisi;

// 王五
extern std::string pubWangwu;
extern std::string privWangwu;

// 赵六
extern std::string pubZhaoliu;
extern std::string privZhaoliu;

// 孙七
extern std::string pubSunqi;
extern std::string privSunqi;