
#include <blockmirror/common.h>
#include <blockmirror/serialization/json_oarchive.h>
#include <blockmirror/store/mongo_store.h>
#include <bsoncxx/json.hpp>

namespace blockmirror {
namespace store {

MongoStore::MongoStore()
    : _client((mongocxx::uri{blockmirror::globalConfig.mongodbURI})) {}

void MongoStore::save(chain::BlockPtr& block) {
  if (!block) return;

  try {
    std::ostringstream oss;
    blockmirror::serialization::JSONOArchive<std::ostringstream> archive(oss,
                                                                         false);

    archive << block;
    bsoncxx::document::value doc = bsoncxx::from_json(oss.str());

    mongocxx::database db = _client[blockmirror::globalConfig.mongodbName];
    mongocxx::collection coll = db[blockmirror::globalConfig.mongodbCollection];
    coll.insert_one(doc.view());
  } catch (std::exception& e) {
    B_WARN("save to mongo exception: {}", e.what());
  }
}

}  // namespace store
}  // namespace blockmirror