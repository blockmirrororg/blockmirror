
#include <blockmirror/common.h>
#include <blockmirror/serialization/bson_oarchive.h>
#include <blockmirror/store/mongo_store.h>
#include <bsoncxx/json.hpp>

namespace blockmirror {
namespace store {

MongoStore::MongoStore()
    : _client((mongocxx::uri{blockmirror::globalConfig.mongodbURI})) {}

void MongoStore::save(chain::BlockPtr& block, chain::Context* context) {
  if (!block) return;

  try {
    auto oss = bsoncxx::builder::stream::document{};
    blockmirror::serialization::BSONOArchive<bsoncxx::builder::stream::document>
        archive(*context, oss);
    archive << block;
    bsoncxx::document::value doc = oss.extract();

    mongocxx::database db = _client[blockmirror::globalConfig.mongodbName];
    mongocxx::collection coll = db[blockmirror::globalConfig.mongodbCollection];
    coll.insert_one(doc.view());
  } catch (std::exception& e) {
    B_ERR("save to mongo exception: {}", e.what());
  }
}

}  // namespace store
}  // namespace blockmirror