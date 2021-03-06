set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(benchmark)
find_package(Threads REQUIRED)


if(benchmark_FOUND)
    add_custom_target(bench)
else()
    message(STATUS "Could not found Google Benchmark. Benchmark targets will be unavailable.")
endif()

function(add_bench_target target_name target_source)

    if(benchmark_FOUND)
        add_executable(${target_name} ${target_source} ${ARGN})
        target_link_libraries(
            ${target_name} benchmark::benchmark implementations
        )
        # The benchmarks might use the internals
        target_include_directories(
            ${target_name}
            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src
        )

        add_dependencies(bench ${target_name})
    endif()
endfunction()

add_bench_target(benchmark_zipf bench_zipf.cpp)
add_bench_target(benchmark_file bench_file.cpp)


add_executable(bench_util bench_util.cpp)
target_link_libraries(bench_util implementations Threads::Threads)
add_sanitizers(bench_util)




find_package(OpenMP)

if(OpenMP_CXX_FOUND)
    add_executable(parallel_read parallel_read.cpp)
    target_link_libraries(parallel_read implementations OpenMP::OpenMP_CXX)
    add_sanitizers(parallel_read)
endif()
