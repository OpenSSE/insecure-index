cmake_minimum_required(VERSION 3.1)
project(
    opensse-insecure-index DESCRIPTION "Implementations of persistent index"
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/externals/sanitizers-cmake/cmake")

find_package(RocksDB REQUIRED)
set(CMAKE_BUILD_RPATH "${HOME}/Code/rocksdb_install/lib")
find_package(WiredTiger REQUIRED)
find_package(spdlog REQUIRED)

# Find Sanitizers
find_package(Sanitizers)

add_compile_options(-fno-rtti)

# From RocksDB's CMakeLists.txt
if(CMAKE_SYSTEM_NAME MATCHES "Cygwin")
  add_definitions(-fno-builtin-memcmp -DCYGWIN)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  add_definitions(-DOS_MACOSX)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
  add_definitions(-DOS_LINUX)
elseif(CMAKE_SYSTEM_NAME MATCHES "SunOS")
  add_definitions(-DOS_SOLARIS)
elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
  add_definitions(-DOS_FREEBSD)
elseif(CMAKE_SYSTEM_NAME MATCHES "NetBSD")
  add_definitions(-DOS_NETBSD)
elseif(CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
  add_definitions(-DOS_OPENBSD)
elseif(CMAKE_SYSTEM_NAME MATCHES "DragonFly")
  add_definitions(-DOS_DRAGONFLYBSD)
elseif(CMAKE_SYSTEM_NAME MATCHES "Android")
  add_definitions(-DOS_ANDROID)
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
  add_definitions(-DWIN32 -DOS_WIN -D_MBCS -DWIN64 -DNOMINMAX)
  if(MINGW)
    add_definitions(-D_WIN32_WINNT=_WIN32_WINNT_VISTA)
  endif()
endif()


add_library(
    implementations
    SHARED
    src/index.cpp
    src/std_multimap.cpp
    src/rocksdb_multimap.cpp
    src/rocksdb_merge_multimap.cpp
    src/wiredtiger_multimap.cpp
    src/utils.cpp
    src/logger.cpp
    src/file_benchmark.cpp
)

target_include_directories(
    implementations
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
    PUBLIC ${ROCKSDB_INCLUDE_DIRS}
    PUBLIC ${WIREDTIGER_INCLUDE_DIRS}
)

target_link_libraries(
    implementations
    ${ROCKSDB_LIBRARIES}
    ${WIREDTIGER_LIBRARIES}
    spdlog::spdlog
)

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
add_sanitizers(check)

target_link_libraries(check gtest_main implementations)

if(${CMAKE_VERSION} VERSION_GREATER "3.10.0")
    gtest_discover_tests(check)
else()
    add_test(check check)
endif()

add_subdirectory(benchmark)

add_executable(playground playground.cpp)
target_link_libraries(playground implementations)
add_sanitizers(playground)
