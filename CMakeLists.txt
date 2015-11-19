project(review-trace CXX)
cmake_minimum_required(VERSION 2.8)

find_package(Boost REQUIRED)

include_directories(.)

add_subdirectory(time)
add_subdirectory(trace)
