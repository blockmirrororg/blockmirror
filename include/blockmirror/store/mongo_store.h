#pragma once

#include <blockmirror/chain/block.h>
#include <blockmirror/chain/context.h>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

namespace blockmirror {
namespace store {

class MongoStore {
 public:
  MongoStore();

  static MongoStore& get() {
    static MongoStore ms;
    return ms;
  }

  void save(chain::BlockPtr& block, chain::Context* context);

 private:
  mongocxx::instance _instance;
  mongocxx::client _client;
};

}  // namespace store
}  // namespace blockmirror