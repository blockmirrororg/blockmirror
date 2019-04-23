
add_library( blockmirror
  secp256k1_contrib/lax_der_parsing.c
  secp256k1_contrib/lax_der_privatekey_parsing.c

  crypto/ecc.cpp
  
  chain/data.cpp
  chain/block.cpp
  chain/transaction.cpp

  store/block_store.cpp
  
  common.cpp
)

target_link_libraries( blockmirror 
   ${Boost_LIBRARIES}
   ${OPENSSL_LIBRARIES}
   ${Secp256k1_LIBRARY}
)
target_include_directories( blockmirror
  PUBLIC "${CMAKE_SOURCE_DIR}/include"
         "${CMAKE_SOURCE_DIR}/include/secp256k1_contrib"
         ${Boost_INCLUDE_DIR}
         ${OPENSSL_INCLUDE_DIR}
         ${Secp256k1_INCLUDE_DIR}
)
