
file(GLOB UNIT_TESTS "*.cpp")

add_executable( unit_test ${UNIT_TESTS} )
target_link_libraries( unit_test blockmirror )
add_test(NAME unit_test COMMAND unit_test WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
