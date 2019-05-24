#pragma once

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <blockmirror/chain/block.h>

namespace blockmirror {
namespace store {

class MongoStore {
 public:
  MongoStore();

  static MongoStore& get() {
    static MongoStore ms;
    return ms;
  }

  void save(chain::BlockPtr& block);

 private:
  mongocxx::instance _instance;
  mongocxx::client _client;
};

}  // namespace store
}  // namespace blockmirror