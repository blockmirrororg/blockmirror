
#include <blockmirror/p2p/channel_manager.h>
#include <blockmirror/server.h>
#include <blockmirror/store/mongo_store.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

namespace blockmirror {

Server::Server()
    : _mainContext(),
      _workContext(),
      _p2pAcceptor(_workContext, blockmirror::globalConfig.p2p_bind),
      _signals(_mainContext),
      _context(),
      _rpcListener(
          _workContext,
          boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(),
                                         blockmirror::globalConfig.rpc_bind},
          _context),
      _producerTimer(_mainContext),
      _strand(_workContext) {}

void Server::handleSignals(int signo) {
  B_LOG("got signo {}", signo);
  stop();
}

void Server::stop() {
  _producerTimer.cancel();
  _mainContext.stop();
  _workContext.stop();
}

void Server::nextProduce(bool tryNow) {
  int delay = _context.getBpsStore().getBPDelay(globalConfig.miner_pubkey);
  if (delay == 0 && tryNow) {
    produceBlock(boost::system::error_code());
    return;
  }
  if (delay <= 0) {
    delay = BLOCK_PER_MS;
  }
  B_LOG("next produce delay: {}", delay);
  _producerTimer.expires_from_now(boost::posix_time::millisec(delay));
  _producerTimer.async_wait(boost::bind(&Server::produceBlock, this,
                                        boost::asio::placeholders::error));
}

void Server::produceBlock(const boost::system::error_code& ec) {
  bool tryNow = true;
  if (ec) {
    B_ERR("produce timer: {}", ec.message());
  } else {
    if (p2p::ChannelManager::get().channelsSyncDone()) {
      auto block = _context.genBlock(globalConfig.miner_privkey,
                                     globalConfig.reward_pubkey);
      if (block) {
        // 在工作线程提交数据到MONGODB
        /*_workContext.post(_strand.wrap(boost::bind(&store::MongoStore::save,
          &_context.getMongoStore(),
          block, &_context)));*/
        _workContext.post(_strand.wrap(boost::bind(&store::MongoStore::save,
                                                   &store::MongoStore::get(),
                                                   block, &_context)));
        tryNow = false;

        const std::vector<boost::shared_ptr<p2p::Channel>> channels =
            p2p::ChannelManager::get().getChannels();
        for (auto i : channels) {
          i->sendGenerateBlock(block);
        }
      }
    } else {
      tryNow = false;
    }
  }
  nextProduce(tryNow);
}

void Server::run() {
  try {
    store::MongoStore::get();  // 先连接测试mongo
  } catch (std::exception& e) {
    throw std::runtime_error(e.what());
  } catch (...) {
    throw std::runtime_error("connect to mongo db failed.");
  }

  for (auto pos = blockmirror::globalConfig.seeds.begin();
       pos != blockmirror::globalConfig.seeds.end(); ++pos) {
    char ip[50] = {0};
    std::size_t index = pos->find(':');
    if (index == std::string::npos) {
      throw std::runtime_error("error seeds");
    }
    strncpy(ip, pos->c_str(), index);
    unsigned short port =
        boost::lexical_cast<unsigned short>(pos->c_str() + index + 1);
    boost::shared_ptr<blockmirror::p2p::Connector> connector =
        boost::make_shared<blockmirror::p2p::Connector>(_workContext, ip, port);
    connector->start();
    _p2pConnecting.push_back(connector);
  }

  _signals.add(SIGINT);
  _signals.add(SIGTERM);
#if defined(SIGQUIT)
  _signals.add(SIGQUIT);
#endif
  // main thread job
  _signals.async_wait(boost::bind(&Server::handleSignals, this,
                                  boost::asio::placeholders::signal_number));

  _context.load();

  if (!_context.getBpsStore().contains(globalConfig.genesis_pubkey)) {
    _context.getBpsStore().add(globalConfig.genesis_pubkey);
  }
  
  _p2pAcceptor.startAccept();
  // rpc
  _rpcListener.run();

  std::vector<boost::shared_ptr<std::thread>> threads;
  for (std::size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
    boost::shared_ptr<std::thread> thread(new std::thread(
        boost::bind(&boost::asio::io_context::run, &_workContext)));
    threads.push_back(thread);
  }

  nextProduce();

  try {
    _mainContext.run();
  } catch (...) {
    std::cerr << "got unknown exception" << std::endl;
  }

  for (auto thread : threads) {
    thread->join();
  }

  _context.close();
}

}  // namespace blockmirror