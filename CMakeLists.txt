
cmake_minimum_required(VERSION 3.00)
project(argparse)

set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# source directories
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src      SRCS)

add_executable(test_argparse ${SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/test/test.cpp)