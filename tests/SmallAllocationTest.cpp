#include "MAlloc/MAlloc.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

#include <gtest/gtest.h>

namespace {

TEST(SmallAllocation, ReturnsNonNullForTinySizes) {
    ma::Allocator allocator;
    for (std::size_t size = 1; size <= 64; ++size) {
        void* ptr = allocator.allocate(size);
        ASSERT_NE(ptr, nullptr);
        allocator.deallocate(ptr);
    }
}

TEST(SmallAllocation, ZeroSizeYieldsValidPointer) {
    ma::Allocator allocator;
    void* ptr = allocator.allocate(0);
    ASSERT_NE(ptr, nullptr);
    allocator.deallocate(ptr);
}

TEST(SmallAllocation, PointersAreDistinct) {
    ma::Allocator allocator;
    std::vector<void*> pointers;
    for (int i = 0; i < 1000; ++i) {
        void* ptr = allocator.allocate(32);
        ASSERT_NE(ptr, nullptr);
        pointers.push_back(ptr);
    }
    std::sort(pointers.begin(), pointers.end());
    ASSERT_EQ(std::adjacent_find(pointers.begin(), pointers.end()), pointers.end());
    for (void* ptr : pointers) {
        allocator.deallocate(ptr);
    }
}

TEST(SmallAllocation, MemoryIsWritable) {
    ma::Allocator allocator;
    for (std::size_t size : {8U, 100U, 512U, 4096U}) {
        auto* ptr = static_cast<std::uint8_t*>(allocator.allocate(size));
        ASSERT_NE(ptr, nullptr);
        std::memset(ptr, 0xAB, size);
        EXPECT_EQ(ptr[0], 0xAB);
        EXPECT_EQ(ptr[size - 1], 0xAB);
        allocator.deallocate(ptr);
    }
}

TEST(SmallAllocation, FreedBlockIsReused) {
    ma::AllocatorConfig config;
    config.enableThreadCache = false;
    ma::Allocator allocator(config);
    void* first = allocator.allocate(64);
    ASSERT_NE(first, nullptr);
    allocator.deallocate(first);
    void* second = allocator.allocate(64);
    EXPECT_EQ(first, second);
    allocator.deallocate(second);
}

TEST(SmallAllocation, AllocationSizeReportsRequest) {
    ma::Allocator allocator;
    void* ptr = allocator.allocate(100);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(allocator.allocationSize(ptr), 100U);
    allocator.deallocate(ptr);
}

TEST(SmallAllocation, TryAllocateSucceeds) {
    ma::Allocator allocator;
    auto result = allocator.tryAllocate(128);
    ASSERT_TRUE(result.has_value());
    ASSERT_NE(*result, nullptr);
    allocator.deallocate(*result);
}

} // namespace
