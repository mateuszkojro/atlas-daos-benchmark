cmake_minimum_required(VERSION 3.15)

project(daos-benchmarking 
        DESCRIPTION "DAOS benchmarks"
        LANGUAGES C CXX
)

add_subdirectory(benchmark)
include_directories(benchmark/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/daos-cxx/include)

add_executable(bench benchmark_main.cxx)
target_link_libraries(bench daos-cxx benchmark::benchmark)
