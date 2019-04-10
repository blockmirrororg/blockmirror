
file(GLOB UNIT_TESTS "*.cpp")

add_executable( unit_test ${UNIT_TESTS} )
target_link_libraries( unit_test blockmirror )
