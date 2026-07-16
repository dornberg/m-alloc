#include "MAlloc/MAlloc.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

TEST(Statistics, CountsAllocationsAndFrees) {
    ma::Allocator allocator;
    void* first = allocator.allocate(100);
    void* second = allocator.allocate(200);
    auto stats = allocator.statistics();
    EXPECT_EQ(stats.allocationCount, 2U);
    EXPECT_EQ(stats.freeCount, 0U);
    EXPECT_EQ(stats.activeAllocations, 2U);
    EXPECT_EQ(stats.currentUsage, 300U);
    EXPECT_EQ(stats.allocatedBytes, 300U);
    allocator.deallocate(first);
    allocator.deallocate(second);
    stats = allocator.statistics();
    EXPECT_EQ(stats.freeCount, 2U);
    EXPECT_EQ(stats.activeAllocations, 0U);
    EXPECT_EQ(stats.currentUsage, 0U);
    EXPECT_EQ(stats.freedBytes, 300U);
}

TEST(Statistics, PeakUsageIsTracked) {
    ma::Allocator allocator;
    void* first = allocator.allocate(1000);
    void* second = allocator.allocate(1000);
    allocator.deallocate(first);
    allocator.deallocate(second);
    const auto stats = allocator.statistics();
    EXPECT_GE(stats.peakUsage, 2000U);
    EXPECT_EQ(stats.currentUsage, 0U);
}

TEST(Statistics, InternalFragmentationIsBounded) {
    ma::Allocator allocator;
    std::vector<void*> pointers;
    for (int i = 0; i < 100; ++i) {
        pointers.push_back(allocator.allocate(100));
    }
    const auto stats = allocator.statistics();
    EXPECT_GE(stats.internalFragmentation, 0.0);
    EXPECT_LT(stats.internalFragmentation, 1.0);
    EXPECT_GE(stats.reservedBytes, stats.currentUsage);
    for (void* ptr : pointers) {
        allocator.deallocate(ptr);
    }
}

TEST(Statistics, FreshAllocatorIsEmpty) {
    ma::Allocator allocator;
    const auto stats = allocator.statistics();
    EXPECT_EQ(stats.allocationCount, 0U);
    EXPECT_EQ(stats.freeCount, 0U);
    EXPECT_EQ(stats.currentUsage, 0U);
    EXPECT_EQ(stats.peakUsage, 0U);
    EXPECT_EQ(stats.externalFragmentation, 0.0);
}

} // namespace
