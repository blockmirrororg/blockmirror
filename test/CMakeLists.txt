
file(GLOB TEST "*.cpp" "*.h")

add_executable(test ${TEST})
target_link_libraries(test blockmirror)