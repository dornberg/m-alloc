add_executable(malloc_benchmarks
    main.cpp
    Adapters.cpp
    Runner.cpp)

target_link_libraries(malloc_benchmarks PRIVATE MAlloc::MAlloc)

set_target_properties(malloc_benchmarks PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)

malloc_set_warnings(malloc_benchmarks ${MALLOC_WARNINGS_AS_ERRORS})

find_package(mimalloc QUIET)
if(mimalloc_FOUND)
    target_link_libraries(malloc_benchmarks PRIVATE mimalloc)
    target_compile_definitions(malloc_benchmarks PRIVATE MALLOC_HAVE_MIMALLOC)
    message(STATUS "Benchmarks: mimalloc enabled")
endif()

find_library(JEMALLOC_LIBRARY jemalloc)
find_path(JEMALLOC_INCLUDE_DIR jemalloc/jemalloc.h)
if(JEMALLOC_LIBRARY AND JEMALLOC_INCLUDE_DIR)
    target_link_libraries(malloc_benchmarks PRIVATE ${JEMALLOC_LIBRARY})
    target_include_directories(malloc_benchmarks PRIVATE ${JEMALLOC_INCLUDE_DIR})
    target_compile_definitions(malloc_benchmarks PRIVATE MALLOC_HAVE_JEMALLOC)
    message(STATUS "Benchmarks: jemalloc enabled")
endif()
