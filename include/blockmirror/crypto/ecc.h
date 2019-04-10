#pragma once

#include <blockmirror/types.h>
#include <secp256k1.h>

namespace blockmirror {
namespace crypto {

class ECCContext {
 private:
  secp256k1_context *_context;
  bool _hasSigner;

 public:
  ECCContext(bool sign = false);
  ~ECCContext();

  bool verify(const Pubkey &pub, const Hash256 &hash, const Signature &sig);
  void sign(const Privkey &priv, const Hash256 &hash, Signature &sig);
  bool verify(const Privkey &priv, const Pubkey &pub);
  void newKey(Privkey &priv);
  void computePub(const Privkey &priv, Pubkey &pub);
};

extern thread_local ECCContext ECC;

}  // namespace crypto
}  // namespace blockmirror
