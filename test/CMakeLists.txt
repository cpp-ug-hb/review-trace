add_executable(benchmark_append
  benchmark_append.cpp
)

target_link_libraries(
  benchmark_append
  PRIVATE
    Trace
    Time
    ${GoogleBenchmark_LIBRARIES}
)
target_include_directories(
  benchmark_append
  PRIVATE
    ${GoogleBenchmark_INCLUDE_DIRS}
)

add_test(benchmark_append benchmark_append)
