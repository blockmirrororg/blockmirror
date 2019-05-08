file(GLOB BLOCKMIRROR_0 "${CMAKE_SOURCE_DIR}/include/blockmirror/*.h")
file(GLOB BLOCKMIRROR_1 "${CMAKE_SOURCE_DIR}/include/blockmirror/store/*.h")
file(GLOB BLOCKMIRROR_2 "${CMAKE_SOURCE_DIR}/include/blockmirror/chain/*.h")
file(GLOB BLOCKMIRROR_3 "${CMAKE_SOURCE_DIR}/include/blockmirror/crypto/*.h")
file(GLOB BLOCKMIRROR_4 "${CMAKE_SOURCE_DIR}/include/blockmirror/p2p/*.h" "${CMAKE_SOURCE_DIR}/src/p2p/*.cpp")
file(GLOB BLOCKMIRROR_5 "${CMAKE_SOURCE_DIR}/include/blockmirror/serialization/*.h")
file(GLOB BLOCKMIRROR_6 "${CMAKE_SOURCE_DIR}/include/blockmirror/rpc/*.h" "${CMAKE_SOURCE_DIR}/src/rpc/*.cpp"
	"${CMAKE_SOURCE_DIR}/src/*.cpp")

add_library( blockmirror
  secp256k1_contrib/lax_der_parsing.c
  secp256k1_contrib/lax_der_privatekey_parsing.c

  crypto/ecc.cpp
  
  chain/data.cpp
  chain/block.cpp
  chain/transaction.cpp

  store/block_store.cpp
  store/format_store.cpp
  store/data_store.cpp
  store/bps_store.cpp
  store/account_store.cpp
  store/transaction_store.cpp
	
	${BLOCKMIRROR_0}
	${BLOCKMIRROR_1}
	${BLOCKMIRROR_2}
	${BLOCKMIRROR_3}
	${BLOCKMIRROR_4}
	${BLOCKMIRROR_5}
	${BLOCKMIRROR_6}

)

target_link_libraries( blockmirror 
   ${ADDITION_LIBRARIES}
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
