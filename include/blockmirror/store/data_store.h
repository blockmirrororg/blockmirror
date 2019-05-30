/**
 * 1. 添加
 * 2. 读取
 * 参考 NewData 交易
 */

#pragma once

#include <blockmirror/chain/script.h>
#include <blockmirror/common.h>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index_container.hpp>

namespace blockmirror {
namespace store {

using NewDataPtr = std::shared_ptr<chain::scri::NewData>;

class DataStore {
private:
  friend class blockmirror::serialization::access;
  template<typename Archive,
    typename std::enable_if<Archive::IsSaving::value, int>::type = 0>
   void serialize(Archive& ar)
  {
    ar << (uint32_t)_container.size();
    for (auto i = _container.begin(); i != _container.end(); ++i)
    {
      ar << *i;
    }
  }

  template<typename Archive,
    typename std::enable_if<!Archive::IsSaving::value, int>::type = 0>
    void serialize(Archive& ar)
  {
    uint32_t size;
    ar >> size;
    if (size > SERIALIZER_MAX_SIZE_T) {
      throw std::runtime_error("TransactionStore::serialize bad size");
    }
    for (uint32_t i = 0; i < size; i++)
    {
      DataItem item(nullptr);
      ar >> item;
      _container.insert(item);
    }
  }

 private:
  //std::unordered_map<std::string, store::NewDataPtr> _datas;

  struct DataItem
  {
    NewDataPtr data;

    DataItem(const NewDataPtr& r) : data(r) {}
    std::string name() const { return data->getName(); }
    std::string format() const { return data->getFormat(); }

  private:
    friend class blockmirror::serialization::access;
    template<typename Archive>
    void serialize(Archive &ar)
    {
      ar & data;
    }
  };

  struct tagName {};
  struct tagFormat {};

  typedef boost::multi_index::multi_index_container<
    DataItem,
    boost::multi_index::indexed_by<
    boost::multi_index::hashed_unique<boost::multi_index::tag<tagName>, BOOST_MULTI_INDEX_CONST_MEM_FUN(DataItem, std::string, name)>,
    boost::multi_index::ordered_non_unique<boost::multi_index::tag<tagFormat>, BOOST_MULTI_INDEX_CONST_MEM_FUN(DataItem, std::string, format)>
    >> DataContainer;

  DataContainer _container;

  // --------------------------------------------------------------------------------
  boost::shared_mutex _mutex;

  boost::filesystem::path _path;
  bool _loaded;

 public:
  DataStore();
  ~DataStore();
  /**
   * @brief 从文件中加载store
   * @param path 路径
   */
  void load(const boost::filesystem::path& path);
  /**
   * @brief 退出程序时需要关闭 主线程 工作线程已关闭
   */
  void close();
  /**
   * @brief 读取数据
   */
  const store::NewDataPtr query(const std::string& name);
  /**
   * @brief 添加数据
   */
  bool add(const store::NewDataPtr& dataPtr);
  /**
   * @brief 删除数据
   */
  bool remove(const std::string& name);

  /**
   * @brief 查找所有的NewData
   */
  std::vector<store::NewDataPtr> queryEx(std::string format);
};

}  // namespace store
}  // namespace blockmirror