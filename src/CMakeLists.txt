
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


target_compile_definitions(blockmirror
  PRIVATE ${LIBMONGOCXX_STATIC_DEFINITIONS} ${LIBBSONCXX_STATIC_DEFINITIONS}
)
target_include_directories( blockmirror
  PUBLIC "${CMAKE_SOURCE_DIR}/include"
         "${CMAKE_SOURCE_DIR}/include/secp256k1_contrib"
         ${Boost_INCLUDE_DIR}
         ${OPENSSL_INCLUDE_DIR}
         ${Secp256k1_INCLUDE_DIR}
         ${LIBMONGOCXX_STATIC_INCLUDE_DIRS}
         ${LIBBSONCXX_STATIC_INCLUDE_DIRS}
         ${BSON_INCLUDE_DIRS}
)

# We can't just use *_STATIC_LIBRARIES variables to link against because the static
# variants of these may try to static link against libraries we don't want (like a system
# libc/c++). But we need to know if mongo c driver was built with ICU, SASL2, or snappy support
# so that we can continue to link to those. This certainly is a bit on the fragile side but
# try to parse what is included in MONGOC_STATIC_LIBRARIES to see what we should link to
foreach(MONGO_S_LIB ${MONGOC_STATIC_LIBRARIES})
  string(REGEX MATCH "libsasl2\\${CMAKE_SHARED_LIBRARY_SUFFIX}$" REGOUT ${MONGO_S_LIB})
  if(REGOUT)
    set(LINK_SASL "sasl2")
  endif()

  string(REGEX MATCH "libicuuc\\${CMAKE_SHARED_LIBRARY_SUFFIX}$" REGOUT ${MONGO_S_LIB})
  if(REGOUT)
    set(LINK_ICU "icuuc")
  endif()

  string(REGEX MATCH "libsnappy\\${CMAKE_SHARED_LIBRARY_SUFFIX}$" REGOUT ${MONGO_S_LIB})
  if(REGOUT)
    set(LINK_SNAPPY "snappy")
  endif()
endforeach()

target_link_libraries( blockmirror 
  ${Boost_LIBRARIES}
  ${Secp256k1_LIBRARY}
  ${LIBMONGOCXX_STATIC_LIBRARY_PATH} ${LIBBSONCXX_STATIC_LIBRARY_PATH}
  ${MONGOC_STATIC_LIBRARY} ${BSON_STATIC_LIBRARY}
  ${LINK_SASL} ${LINK_ICU} ${LINK_SNAPPY}
  ${OPENSSL_LIBRARIES}
  ${ADDITION_LIBRARIES}
)