include(ExternalProject)

ExternalProject_add(
  GoogleBenchmark
  URL https://github.com/google/benchmark/archive/v0.1.0.tar.gz
  URL_MD5 5a3ec4924f75f98cf01a8f1ff47d7a44
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
)

ExternalProject_Get_Property(GoogleBenchmark INSTALL_DIR)

message(STATUS "INSTALL_DIR: ${INSTALL_DIR}")
set(GoogleBenchmark_LIBRARIES ${INSTALL_DIR}/lib/libbenchmark.a)
set(GoogleBenchmark_INCLUDE_DIRS ${INSTALL_DIR}/include)
