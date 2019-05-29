#pragma once

#include <blockmirror/chain/block.h>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

namespace blockmirror {

namespace chain {
class Context;
}

namespace store {

class MongoStore {
 public:
  MongoStore();

  void save(chain::BlockPtr& block, chain::Context* context);

 private:
  mongocxx::instance _instance;
  mongocxx::client _client;
};

}  // namespace store
}  // namespace blockmirror