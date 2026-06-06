cmake_minimum_required(VERSION 3.10)
project(zkint LANGUAGES C CXX)

if(NOT CMAKE_C_STANDARD)
    set(CMAKE_C_STANDARD 99)
endif()
set(CMAKE_C_STANDARD_REQUIRED ON)
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(zkint INTERFACE)
add_library(zk::zkint ALIAS zkint)

target_include_directories(zkint INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

option(ZKINT_BUILD_TESTS "Build tests for zkint" ON)
if(ZKINT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
