cmake_minimum_required(VERSION 3.1)
project(
    opensse-insecure-index DESCRIPTION "Implementations of persistent index"
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(RocksDB REQUIRED)

add_library(
    implementations
    SHARED
    src/std_multimap.cpp
    src/rocksdb_multimap.cpp
    src/utils.cpp
)

target_include_directories(
    implementations
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
    PUBLIC ${ROCKSDB_INCLUDE_DIRS}
)

target_link_libraries(implementations ${ROCKSDB_LIBRARIES})

target_compile_features(
    implementations
    PUBLIC
    cxx_auto_type
    cxx_constexpr
    cxx_defaulted_functions
)

enable_testing()

# Build googletest for the tests
set(BUILD_GMOCK OFF CACHE BOOL "Disable GMock" FORCE)
add_subdirectory(externals/googletest)

if(${CMAKE_VERSION} VERSION_GREATER "3.10.0")
    include(GoogleTest)
endif()

add_executable(check test/index_test.cpp test/zipf_test.cpp test/utility.cpp)

target_link_libraries(check gtest_main implementations)

if(${CMAKE_VERSION} VERSION_GREATER "3.10.0")
    gtest_discover_tests(check)
else()
    add_test(check check)
endif()


add_subdirectory(benchmark)
