
cmake_minimum_required(VERSION 2.8.7)
message(STATUS "CMake version ${CMAKE_VERSION}")

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_SOURCE_DIR}/cmake")

project(blockmirror)

set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_CXX_EXTENSIONS ON )
set( CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS
    date_time
    filesystem
    serialization
    unit_test_framework)

find_package(OpenSSL REQUIRED)

find_package(secp256k1 REQUIRED)

add_subdirectory( src )
add_subdirectory( unittests )