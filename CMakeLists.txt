project(review-trace CXX)
cmake_minimum_required(VERSION 2.8)

find_package(Boost REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

include(cmake/GoogleBenchmark.cmake)
include(CTest)

include_directories(.)

add_subdirectory(time)
add_subdirectory(trace)
add_subdirectory(test)
