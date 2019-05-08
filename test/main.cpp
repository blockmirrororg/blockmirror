
#include <boost/asio.hpp>

#include <boost/lexical_cast.hpp>
#include <iostream>
#include "server.h"

int main(int argc, char* argv[]) {
  try {
    std::size_t num_threads = 4;
    if (argc == 2) {
      num_threads = boost::lexical_cast<std::size_t>(argv[1]);
    }

    Server server(num_threads);
    server.run();
  } catch (std::exception& e) {
    std::cerr << "exception: " << e.what() << "\n";
  }

  return 0;
}