#include <blockmirror/crypto/ecc.h>

#include <openssl/rand.h>
#include <openssl/sha.h>
#include <secp256k1_contrib/lax_der_parsing.h>
#include <boost/endian/conversion.hpp>
#include <cassert>

namespace blockmirror {
namespace crypto {

thread_local ECCContext ECC(true);

ECCContext::ECCContext(bool sign)
    : _hasSigner(sign),
      _context(secp256k1_context_create(
          sign ? (SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY)
               : SECP256K1_CONTEXT_VERIFY)) {
  assert(_context);
  if (_hasSigner) {
    unsigned char vseed[32];
    int r = RAND_bytes(vseed, 32);
    if (r != 1) {
      assert(false && "ERR_get_error");
    }
    r = secp256k1_context_randomize(_context, vseed);
    assert(r);
  }
}

ECCContext::~ECCContext() { secp256k1_context_destroy(_context); }

bool ECCContext::verify(const Pubkey &pub, const Hash256 &hash,
                        const Signature &sig) const {
  secp256k1_pubkey pubkey;
  secp256k1_ecdsa_signature signature;
  if (!secp256k1_ec_pubkey_parse(_context, &pubkey, pub.data(), pub.size())) {
    printf("secp256k1_ec_pubkey_parse\n");
    return false;
  }
  if (!ecdsa_signature_parse_der_lax(_context, &signature, sig.data(),
                                     sig.size())) {
    printf("ecdsa_signature_parse_der_lax\n");
    return false;
  }
  secp256k1_ecdsa_signature_normalize(_context, &signature, &signature);
  return secp256k1_ecdsa_verify(_context, &signature, hash.data(), &pubkey);
}

// Check that the sig has a low R value and will be less than 71 bytes
bool SigHasLowR(const secp256k1_context *ctx,
                const secp256k1_ecdsa_signature *sig) {
  unsigned char compact_sig[64];
  secp256k1_ecdsa_signature_serialize_compact(ctx, compact_sig, sig);

  // In DER serialization, all values are interpreted as big-endian, signed
  // integers. The highest bit in the integer indicates its signed-ness; 0 is
  // positive, 1 is negative. When the value is interpreted as a negative
  // integer, it must be converted to a positive value by prepending a 0x00 byte
  // so that the highest bit is 0. We can avoid this prepending by ensuring that
  // our highest bit is always 0, and thus we must check that the first byte is
  // less than 0x80.
  return compact_sig[0] < 0x80;
}

bool ECCContext::verify(const Privkey &priv, const Pubkey &pub) const {
  assert(_hasSigner);
  unsigned char rnd[8];
  std::string str = "Blockmirror key verification\n";
  RAND_bytes(rnd, sizeof(rnd));

  Hash256 hash;
  SHA256_CTX ctx;
  SHA256_Init(&ctx);
  SHA256_Update(&ctx, str.data(), str.size());
  SHA256_Update(&ctx, rnd, sizeof(rnd));
  SHA256_Final(hash.data(), &ctx);

  Signature sig;
  sign(priv, hash, sig);
  return verify(pub, hash, sig);
}

void ECCContext::sign(const Privkey &priv, const Hash256 &hash,
                      Signature &sig) const {
  assert(_hasSigner);
  secp256k1_ecdsa_signature signature;
  union {
    unsigned char extra_entropy[32] = {0};
    uint32_t counter;
  };
  size_t sigLen = sig.size();
  int ret = secp256k1_ecdsa_sign(_context, &signature, hash.data(), priv.data(),
                                 secp256k1_nonce_function_rfc6979, nullptr);

  while (ret && !SigHasLowR(_context, &signature)) {
    ++counter;
    boost::endian::native_to_little_inplace(counter);
    ret = secp256k1_ecdsa_sign(_context, &signature, hash.data(), priv.data(),
                               secp256k1_nonce_function_rfc6979, extra_entropy);
    boost::endian::little_to_native_inplace(counter);
  }
  assert(ret);
  secp256k1_ecdsa_signature_serialize_der(_context, sig.data(), &sigLen,
                                          &signature);
  assert(sigLen != sig.size());
}

void ECCContext::newKey(Privkey &priv) const {
  assert(_hasSigner);
  do {
    RAND_bytes(priv.data(), priv.size());
  } while (!secp256k1_ec_seckey_verify(_context, priv.data()));
}

void ECCContext::computePub(const Privkey &priv, Pubkey &pub) const {
  assert(_hasSigner);
  secp256k1_pubkey pubkey;
  size_t clen = pub.size();
  int ret = secp256k1_ec_pubkey_create(_context, &pubkey, priv.data());
  assert(ret);
  secp256k1_ec_pubkey_serialize(_context, pub.data(), &clen, &pubkey,
                                SECP256K1_EC_COMPRESSED);
  assert(pub.size() == clen);
}

}  // namespace crypto
}  // namespace blockmirror
