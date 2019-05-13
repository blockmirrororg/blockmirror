
#include <boost/asio.hpp>

#include <blockmirror/server.h>
#include <iostream>

int main(int argc, char* argv[]) {
  try {
    blockmirror::Server server;
    server.add_connector("127.0.0.1", 7700);  // for p2p
    server.add_connector("127.0.0.1", 7701);
    server.run();
  } catch (std::exception& e) {
    std::cerr << "exception: " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "unknown exception" << std::endl;
  }

  return 0;
}