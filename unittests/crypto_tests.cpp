
#include <blockmirror/crypto/ecc.h>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(crypto_tests)

BOOST_AUTO_TEST_CASE(crypto_tests_ecc) {
  blockmirror::Privkey key1 = {0x12, 0xb0, 0x04, 0xff, 0xf7, 0xf4, 0xb6, 0x9e,
                               0xf8, 0x65, 0x0e, 0x76, 0x7f, 0x18, 0xf1, 0x1e,
                               0xde, 0x15, 0x81, 0x48, 0xb4, 0x25, 0x66, 0x07,
                               0x23, 0xb9, 0xf9, 0xa6, 0x6e, 0x61, 0xf7, 0x47};
  blockmirror::Privkey key2 = {0xb5, 0x24, 0xc2, 0x8b, 0x61, 0xc9, 0xb2, 0xc4,
                               0x9b, 0x2c, 0x7d, 0xd4, 0xc2, 0xd7, 0x58, 0x87,
                               0xab, 0xb7, 0x87, 0x68, 0xc0, 0x54, 0xbd, 0x7c,
                               0x01, 0xaf, 0x40, 0x29, 0xf6, 0xc0, 0xd1, 0x17};
  blockmirror::Pubkey pub1 = {
      0x03, 0x0b, 0x4c, 0x86, 0x65, 0x85, 0xdd, 0x86, 0x8a, 0x9d, 0x62,
      0x34, 0x8a, 0x9c, 0xd0, 0x08, 0xd6, 0xa3, 0x12, 0x93, 0x70, 0x48,
      0xff, 0xf3, 0x16, 0x70, 0xe7, 0xe9, 0x20, 0xcf, 0xc7, 0xa7, 0x44};
  blockmirror::Pubkey pub2 = {
      0x03, 0x18, 0x39, 0x05, 0xae, 0x25, 0xe8, 0x15, 0x63, 0x4c, 0xe7,
      0xf5, 0xd9, 0xbe, 0xdb, 0xaa, 0x2c, 0x39, 0x03, 0x2a, 0xb9, 0x8c,
      0x75, 0xb5, 0xe8, 0x8f, 0xe4, 0x3f, 0x8d, 0xd8, 0x24, 0x6f, 0x3c};
  blockmirror::Hash256 hash = {0xfb, 0xd9, 0xcc, 0x55, 0x0a, 0xd5, 0xce, 0x6d,
                               0x3c, 0x5e, 0xb5, 0x3c, 0x0b, 0x54, 0x26, 0x90,
                               0x87, 0xd3, 0x8c, 0x49, 0x31, 0x6d, 0x94, 0xf2,
                               0xca, 0xe3, 0xbf, 0xf2, 0x4d, 0x18, 0x9b, 0xc2};
  blockmirror::Hash256 hash2 = {0x52, 0x55, 0x68, 0x3d, 0xa5, 0x67, 0x90, 0x0b,
                                0xfd, 0x3e, 0x78, 0x6e, 0xd8, 0x83, 0x6a, 0x4e,
                                0x77, 0x63, 0xc2, 0x21, 0xbf, 0x1a, 0xc2, 0x0e,
                                0xce, 0x2a, 0x51, 0x71, 0xb9, 0x19, 0x9e, 0x8a};
  blockmirror::Signature sig1 = {
      0x30, 0x44, 0x02, 0x20, 0x30, 0x37, 0x97, 0x35, 0xa7, 0xe7, 0xb7, 0x05,
      0xf7, 0xd4, 0x7e, 0x7c, 0x68, 0x09, 0xa3, 0x6b, 0xba, 0x5d, 0xd0, 0xa2,
      0xe5, 0x40, 0xe7, 0x56, 0x57, 0xf4, 0xeb, 0xfe, 0x18, 0x1d, 0xb3, 0x52,
      0x02, 0x20, 0x33, 0x65, 0x0f, 0xc5, 0x6a, 0xe4, 0xef, 0x65, 0x90, 0x59,
      0x6e, 0x15, 0x0b, 0xa5, 0xb5, 0x71, 0x7e, 0xca, 0x0a, 0xe2, 0x70, 0x73,
      0x29, 0xe0, 0xe2, 0xbc, 0x85, 0x2d, 0x3c, 0x1b, 0x4d, 0x6f};
  blockmirror::Signature sig2 = {
      0x30, 0x44, 0x02, 0x20, 0x27, 0xf6, 0xf1, 0x14, 0x64, 0xe4, 0xf0, 0x98,
      0x7d, 0x8f, 0xd9, 0x73, 0x88, 0x49, 0x6f, 0x79, 0x6d, 0x70, 0xcc, 0xd0,
      0xfd, 0x93, 0xeb, 0xf7, 0x8e, 0x24, 0xc7, 0x79, 0x7c, 0xe9, 0xbc, 0x6f,
      0x02, 0x20, 0x07, 0x6a, 0xd6, 0x9b, 0x35, 0x0f, 0x49, 0xe5, 0x20, 0xc5,
      0xc1, 0x3a, 0x2a, 0xb6, 0x72, 0xfa, 0x69, 0x88, 0x21, 0xaa, 0x81, 0x63,
      0xe7, 0x69, 0x67, 0x6c, 0x25, 0xf5, 0x89, 0x3e, 0x32, 0x29};

  blockmirror::Signature dsig1 = {
      0x30, 0x44, 0x02, 0x20, 0x5d, 0xbb, 0xdd, 0xda, 0x71, 0x77, 0x2d, 0x95,
      0xce, 0x91, 0xcd, 0x2d, 0x14, 0xb5, 0x92, 0xcf, 0xbc, 0x1d, 0xd0, 0xaa,
      0xbd, 0x6a, 0x39, 0x4b, 0x6c, 0x2d, 0x37, 0x7b, 0xbe, 0x59, 0xd3, 0x1d,
      0x02, 0x20, 0x14, 0xdd, 0xda, 0x21, 0x49, 0x4a, 0x4e, 0x22, 0x1f, 0x08,
      0x24, 0xf0, 0xb8, 0xb9, 0x24, 0xc4, 0x3f, 0xa4, 0x3c, 0x0a, 0xd5, 0x7d,
      0xcc, 0xda, 0xa1, 0x1f, 0x81, 0xa6, 0xbd, 0x45, 0x82, 0xf6};
  blockmirror::Signature dsig2 = {
      0x30, 0x44, 0x02, 0x20, 0x52, 0xd8, 0xa3, 0x20, 0x79, 0xc1, 0x1e, 0x79,
      0xdb, 0x95, 0xaf, 0x63, 0xbb, 0x96, 0x00, 0xc5, 0xb0, 0x4f, 0x21, 0xa9,
      0xca, 0x33, 0xdc, 0x12, 0x9c, 0x2b, 0xfa, 0x8a, 0xc9, 0xdc, 0x1c, 0xd5,
      0x02, 0x20, 0x61, 0xd8, 0xae, 0x5e, 0x0f, 0x6c, 0x1a, 0x16, 0xbd, 0xe3,
      0x71, 0x9c, 0x64, 0xc2, 0xfd, 0x70, 0xe4, 0x04, 0xb6, 0x42, 0x8a, 0xb9,
      0xa6, 0x95, 0x66, 0x96, 0x2e, 0x87, 0x71, 0xb5, 0x94, 0x4d};

  using blockmirror::crypto::ECC;

  BOOST_CHECK(ECC.verify(key1, pub1));
  BOOST_CHECK(ECC.verify(key2, pub2));
  BOOST_CHECK(ECC.verify(pub1, hash, sig1));
  BOOST_CHECK(ECC.verify(pub2, hash, sig2));
  BOOST_CHECK(ECC.verify(pub1, hash2, dsig1));
  BOOST_CHECK(ECC.verify(pub2, hash2, dsig2));

  blockmirror::Pubkey pub11, pub22;
  ECC.computePub(key1, pub11);
  ECC.computePub(key2, pub22);
  BOOST_CHECK_EQUAL_COLLECTIONS(pub11.begin(), pub11.end(), pub1.begin(),
                                pub1.end());
  BOOST_CHECK_EQUAL_COLLECTIONS(pub22.begin(), pub22.end(), pub2.begin(),
                                pub2.end());

  blockmirror::Signature sig11, sig22;
  ECC.sign(key1, hash, sig11);
  ECC.sign(key2, hash, sig22);
  BOOST_CHECK_EQUAL_COLLECTIONS(sig11.begin(), sig11.end(), sig1.begin(),
                                sig1.end());
  BOOST_CHECK_EQUAL_COLLECTIONS(sig22.begin(), sig22.end(), sig2.begin(),
                                sig2.end());

  ECC.sign(key1, hash2, sig11);
  ECC.sign(key2, hash2, sig22);
  BOOST_CHECK_EQUAL_COLLECTIONS(sig11.begin(), sig11.end(), dsig1.begin(),
                                dsig1.end());
  BOOST_CHECK_EQUAL_COLLECTIONS(sig22.begin(), sig22.end(), dsig2.begin(),
                                dsig2.end());
}

BOOST_AUTO_TEST_SUITE_END()
