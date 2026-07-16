include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(malloc_tests
    AlignmentTest.cpp
    DebugFeaturesTest.cpp
    FragmentationTest.cpp
    LargeAllocationTest.cpp
    RandomWorkloadTest.cpp
    ReallocateTest.cpp
    SmallAllocationTest.cpp
    StatisticsTest.cpp
    StlIntegrationTest.cpp
    StressTest.cpp
    ThreadSafetyTest.cpp)

target_link_libraries(malloc_tests PRIVATE MAlloc::MAlloc GTest::gtest_main)

set_target_properties(malloc_tests PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)

malloc_set_warnings(malloc_tests ${MALLOC_WARNINGS_AS_ERRORS})

if(MALLOC_ENABLE_SANITIZERS)
    malloc_enable_sanitizers(malloc_tests)
endif()

include(GoogleTest)
gtest_discover_tests(malloc_tests DISCOVERY_TIMEOUT 60)
