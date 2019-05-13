
file(GLOB p2p				${CMAKE_SOURCE_DIR}/src/p2p/*.cpp ${CMAKE_SOURCE_DIR}/include/blockmirror/p2p/*.h)
file(GLOB rpc				${CMAKE_SOURCE_DIR}/src/rpc/*.cpp ${CMAKE_SOURCE_DIR}/include/blockmirror/rpc/*.h)
file(GLOB chain				${CMAKE_SOURCE_DIR}/src/chain/*.cpp ${CMAKE_SOURCE_DIR}/include/blockmirror/chain/*.h)
file(GLOB crypto			${CMAKE_SOURCE_DIR}/src/crypto/*.cpp ${CMAKE_SOURCE_DIR}/include/blockmirror/crypto/*.h)
file(GLOB store				${CMAKE_SOURCE_DIR}/src/store/*.cpp ${CMAKE_SOURCE_DIR}/include/blockmirror/store/*.h)
file(GLOB serialization		${CMAKE_SOURCE_DIR}/include/blockmirror/serialization/*.h)
file(GLOB common			${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/include/blockmirror/*.h)
file(GLOB secp256k1_contrib	${CMAKE_SOURCE_DIR}/src/secp256k1_contrib/*.c ${CMAKE_SOURCE_DIR}/include/secp256k1_contrib/*.h)

add_library( blockmirror
  ${secp256k1_contrib}
  ${p2p}
  ${rpc}
  ${chain}
  ${crypto}
  ${store}
  ${serialization}
  ${common}
)

source_group(p2p				FILES ${p2p})
source_group(rpc				FILES ${rpc})
source_group(chain				FILES ${chain})
source_group(store				FILES ${store})
source_group(serialization		FILES ${serialization})
source_group(crypto				FILES ${crypto})
source_group(secp256k1_contrib	FILES ${secp256k1_contrib})
source_group(""					FILES ${common})

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
