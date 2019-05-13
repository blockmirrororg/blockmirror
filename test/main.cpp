
#include <boost/asio.hpp>

#include <blockmirror/server.h>
#include <iostream>

int main(int argc, char* argv[]) {
  try {
    B_LOG("blockmirror server starting...");
    blockmirror::Server server;
    server.add_connector("127.0.0.1", 7700);  // for p2p
    server.add_connector("127.0.0.1", 7701);
    server.run();
  } catch (std::exception& e) {
    B_ERR("exception: {}", e.what());
  } catch (...) {
    B_ERR("unknown exception");
  }

  return 0;
}