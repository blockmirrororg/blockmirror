
file(GLOB TEST "*.cpp" "*.h")

add_executable(test_network ${TEST})
target_link_libraries(test_network blockmirror)