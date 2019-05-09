
#include <boost/asio.hpp>

#include <boost/lexical_cast.hpp>
#include <iostream>
#include <blockmirror/server.h>

int main(int argc, char* argv[]) {
  try {
    std::size_t num_threads = 4;
    if (argc == 2) {
      num_threads = boost::lexical_cast<std::size_t>(argv[1]);
    }

    blockmirror::Server server(num_threads);
	server.add_connector("127.0.0.1", 7700); // for p2p
	server.add_connector("127.0.0.1", 7701);
    server.run();
  } catch (std::exception& e) {
    std::cerr << "exception: " << e.what() << "\n";
  }

  return 0;
}