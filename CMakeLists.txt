cmake_minimum_required(VERSION 3.15)
project(LowLatencyBus CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")

include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(lowlatencybus INTERFACE)
target_include_directories(lowlatencybus INTERFACE ${CMAKE_SOURCE_DIR}/include)

add_executable(publisher examples/publisher.cpp)
target_link_libraries(publisher lowlatencybus)

add_executable(subscriber examples/subscriber.cpp)
target_link_libraries(subscriber lowlatencybus)

add_executable(benchmark tools/benchmark.cpp)
target_link_libraries(benchmark lowlatencybus)

add_executable(test_bus tests/integration_test.cpp)
target_link_libraries(test_bus lowlatencybus)

enable_testing()
add_test(NAME IntegrationTest COMMAND test_bus)
