add_executable(test_c test_c.c)
target_link_libraries(test_c PRIVATE zkint)

add_executable(test_cpp test_cpp.cpp)
target_link_libraries(test_cpp PRIVATE zkint)

add_executable(test_perf test_perf.cpp)
target_link_libraries(test_perf PRIVATE zkint)

add_test(NAME test_c COMMAND test_c)
add_test(NAME test_cpp COMMAND test_cpp)
