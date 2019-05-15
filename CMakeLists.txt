
cmake_minimum_required(VERSION 2.8.7)
message(STATUS "CMake version ${CMAKE_VERSION}")
cmake_policy(SET CMP0037 NEW)

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_SOURCE_DIR}/cmake")

project(blockmirror)

set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_CXX_EXTENSIONS ON )
set( CXX_STANDARD_REQUIRED ON )

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    add_compile_options("/utf-8")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MT")
    
    set(Secp256k1_ROOT_DIR "C:/Users/Administrator/vcpkg/installed/x64-windows-static")
    set(OPENSSL_ROOT_DIR "C:/Users/Administrator/vcpkg/installed/x64-windows-static")
    set(BOOST_ROOT "C:/Users/Administrator/vcpkg/installed/x64-windows-static")

	set(spdlog_DIR "C:/Users/Administrator/vcpkg/installed/x64-windows-static/share/spdlog")
	set(fmt_DIR "C:/Users/Administrator/vcpkg/installed/x64-windows-static/share/fmt")
    set(ADDITION_LIBRARIES spdlog)
else()
    set(ADDITION_LIBRARIES pthread spdlog)
endif()

set(CMAKE_BUILD_TYPE "Debug")

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

find_package(secp256k1 REQUIRED)
#find_package(spdlog REQUIRED)

add_subdirectory( 3dparty )
add_subdirectory( src )
add_subdirectory( unittests )
add_subdirectory( test )