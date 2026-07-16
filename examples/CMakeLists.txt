function(malloc_add_example name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE MAlloc::MAlloc)
    set_target_properties(${name} PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)
    malloc_set_warnings(${name} ${MALLOC_WARNINGS_AS_ERRORS})
endfunction()

malloc_add_example(basic_allocation)
malloc_add_example(placement_new)
malloc_add_example(stl_integration)
malloc_add_example(custom_allocator)
malloc_add_example(benchmark_example)
malloc_add_example(statistics_example)
