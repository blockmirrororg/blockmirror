
cmake_minimum_required(VERSION 2.8.7)
message(STATUS "CMake version ${CMAKE_VERSION}")

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_SOURCE_DIR}/cmake")

project(blockmirror)

set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_CXX_EXTENSIONS ON )
set( CXX_STANDARD_REQUIRED ON)

#set(Secp256k1_ROOT_DIR "C:/Users/Administrator/vcpkg/installed/x64-windows-static")
#set(OPENSSL_ROOT_DIR "C:\\Users\\Administrator\\vcpkg\\installed\\x64-windows-static")
#set(BOOST_ROOT "C:\\Users\\Administrator\\vcpkg\\installed\\x64-windows-static")

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MT")
#set(CMAKE_BUILD_TYPE "Debug")

find_package(Boost REQUIRED COMPONENTS
    date_time
    filesystem
    serialization
    thread
    unit_test_framework)

find_package(OpenSSL REQUIRED)

find_package(secp256k1 REQUIRED)

add_subdirectory( src )
add_subdirectory( unittests )