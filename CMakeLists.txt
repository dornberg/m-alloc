cmake_minimum_required(VERSION 3.25)

project(MAlloc
    VERSION 0.1.0
    DESCRIPTION "Modern high-performance cross-platform memory allocator"
    HOMEPAGE_URL "https://github.com/dornberg/m-alloc"
    LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CompilerWarnings)
include(GNUInstallDirs)

if(PROJECT_IS_TOP_LEVEL)
    set(MALLOC_IS_TOP_LEVEL ON)
else()
    set(MALLOC_IS_TOP_LEVEL OFF)
endif()

option(MALLOC_BUILD_TESTS "Build unit tests" ${MALLOC_IS_TOP_LEVEL})
option(MALLOC_BUILD_BENCHMARKS "Build benchmarks" ${MALLOC_IS_TOP_LEVEL})
option(MALLOC_BUILD_EXAMPLES "Build examples" ${MALLOC_IS_TOP_LEVEL})
option(MALLOC_ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)
option(MALLOC_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

add_library(malloc_lib
    src/Allocator.cpp
    src/AllocatorImpl.cpp
    src/LargeAllocator.cpp
    src/PoolAllocator.cpp
    src/SegmentHeap.cpp
    src/SystemMemory.cpp
    src/ThreadCache.cpp)

add_library(MAlloc::MAlloc ALIAS malloc_lib)

set_target_properties(malloc_lib PROPERTIES
    OUTPUT_NAME malloc
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON)

target_include_directories(malloc_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_compile_features(malloc_lib PUBLIC cxx_std_23)

malloc_set_warnings(malloc_lib ${MALLOC_WARNINGS_AS_ERRORS})

if(MALLOC_ENABLE_SANITIZERS)
    include(Sanitizers)
    malloc_enable_sanitizers(malloc_lib)
endif()

find_package(Threads REQUIRED)
target_link_libraries(malloc_lib PUBLIC Threads::Threads)

if(MALLOC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(MALLOC_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(MALLOC_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

include(CMakePackageConfigHelpers)

install(TARGETS malloc_lib
    EXPORT MAllocTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY include/MAlloc DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT MAllocTargets
    FILE MAllocTargets.cmake
    NAMESPACE MAlloc::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MAlloc)

configure_package_config_file(
    cmake/MAllocConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/MAllocConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MAlloc)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/MAllocConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/MAllocConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/MAllocConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MAlloc)
