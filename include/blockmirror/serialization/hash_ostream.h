#pragma once

#include <blockmirror/types.h>
#include <openssl/sha.h>

namespace blockmirror {
namespace serialization {

class HashOStream {
 private:
  SHA256_CTX _ctx;

 public:
  HashOStream() { SHA256_Init(&_ctx); }

  void getHash(Hash256 &hash) {
    SHA256_Final((unsigned char *)hash.data(), &_ctx);
  }

  void reset() { SHA256_Init(&_ctx); }

  void write(char *buf, size_t size) { SHA256_Update(&_ctx, buf, size); }
};

}  // namespace serialization
}  // namespace blockmirror
