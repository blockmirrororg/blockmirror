#pragma once

#include <blockmirror/types.h>
#include <secp256k1.h>
#include <vector>

namespace blockmirror {
namespace crypto {

class ECCContext {
 private:
  secp256k1_context *_context;
  bool _hasSigner;

 public:
  ECCContext(bool sign = false);
  ~ECCContext();

  bool verify(const Pubkey &pub, const Hash256 &hash,
              const Signature &sig) const;
  void sign(const Privkey &priv, const Hash256 &hash, Signature &sig) const;
  bool verify(const Privkey &priv, const Pubkey &pub) const;
  void newKey(Privkey &priv) const;
  void computePub(const Privkey &priv, Pubkey &pub) const;

  void normalizeSignature(const std::vector<uint8_t> &in, Signature &sig) const;
};

extern thread_local ECCContext ECC;

}  // namespace crypto
}  // namespace blockmirror
