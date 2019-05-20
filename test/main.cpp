
#include <blockmirror/server.h>
#include <iostream>

int main(int argc, char* argv[]) {
  try {
    B_LOG("blockmirror server starting...");
    if (argc != 2) {
      B_ERR("usage: {} CONFIG_FILE", argv[0]);
      return -1;
    }

    blockmirror::globalConfig.init(argv[1]);

    blockmirror::Server server;
    server.run();
  } catch (std::exception& e) {
    B_ERR("exception: {}", e.what());
  } catch (...) {
    B_ERR("unknown exception");
  }

  return 0;
}