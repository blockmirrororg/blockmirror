
cmake_minimum_required(VERSION 2.8.7)
message(STATUS "CMake version ${CMAKE_VERSION}")
cmake_policy(SET CMP0037 NEW)
cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0054 NEW)

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_SOURCE_DIR}/cmake")

project(blockmirror)

set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_CXX_EXTENSIONS ON )
set( CXX_STANDARD_REQUIRED ON )

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    add_compile_options("/utf-8")
    # 这里只能连接VC的MT库，因为find_package不能找到第三方的debug库 MTd会冲突
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MT")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    add_definitions(-D_WIN32_WINNT=0x0600)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    
    if("${BOOST_ROOT}" STREQUAL "")
        set(BOOST_ROOT "${ADDITION_ROOT}")
    endif()
    set(Secp256k1_ROOT_DIR "${ADDITION_ROOT}")
    set(OPENSSL_ROOT_DIR "${ADDITION_ROOT}")
    set(libmongoc-static-1.0_DIR "${ADDITION_ROOT}/share/libmongoc-static-1.0")
    set(libbson-static-1.0_DIR "${ADDITION_ROOT}/share/libbson-static-1.0")
    
    add_definitions(-DBSONCXX_STATIC)
    add_definitions(-DMONGOCXX_STATIC)
    set(ADDITION_LIBRARIES spdlog ${ADDITION_ROOT}/lib/libmongocxx.lib ${ADDITION_ROOT}/lib/bsoncxx.lib
         Dnsapi.lib Secur32.lib bcrypt.lib Crypt32.lib)
else()
    if(APPLE) 
        set(ADDITION_LIBRARIES pthread spdlog dl z resolv)
    else()
        set(ADDITION_LIBRARIES pthread spdlog dl rt z resolv)
    endif()
    find_package(libbsoncxx-static REQUIRED)
    find_package(libmongocxx-static REQUIRED)
endif()

add_definitions(-DBOOST_ENABLE_ASSERT_HANDLER)
set(CMAKE_BUILD_TYPE "DEBUG")
set(Boost_NO_BOOST_CMAKE ON)
find_package(Boost 1.67 REQUIRED COMPONENTS
    thread
    date_time
    filesystem
    system
    serialization
    chrono
    unit_test_framework)

find_package(OpenSSL REQUIRED)

find_package(Secp256k1 REQUIRED)

find_package(libmongoc-static-1.0 REQUIRED)
find_package(libbson-static-1.0 REQUIRED)

add_subdirectory( 3dparty )
add_subdirectory( src )
include(CTest)
add_subdirectory( unittests )
add_subdirectory( test )