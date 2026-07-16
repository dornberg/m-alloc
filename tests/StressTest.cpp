#include "MAlloc/MAlloc.hpp"

#include <cstddef>
#include <vector>

#include <gtest/gtest.h>

namespace {

TEST(Stress, RapidAllocateFreeCycles) {
    ma::Allocator allocator;
    for (int cycle = 0; cycle < 200; ++cycle) {
        std::vector<void*> pointers;
        pointers.reserve(500);
        for (int i = 0; i < 500; ++i) {
            const std::size_t size = static_cast<std::size_t>(i % 300) + 1;
            void* ptr = allocator.allocate(size);
            ASSERT_NE(ptr, nullptr);
            pointers.push_back(ptr);
        }
        for (void* ptr : pointers) {
            allocator.deallocate(ptr);
        }
    }
    const auto stats = allocator.statistics();
    EXPECT_EQ(stats.activeAllocations, 0U);
}

TEST(Stress, ManySimultaneousSmallAllocations) {
    ma::Allocator allocator;
    constexpr int kCount = 100000;
    std::vector<void*> pointers;
    pointers.reserve(kCount);
    for (int i = 0; i < kCount; ++i) {
        void* ptr = allocator.allocate(static_cast<std::size_t>(i % 128) + 1);
        ASSERT_NE(ptr, nullptr);
        pointers.push_back(ptr);
    }
    for (void* ptr : pointers) {
        allocator.deallocate(ptr);
    }
    const auto stats = allocator.statistics();
    EXPECT_EQ(stats.allocationCount, static_cast<std::uint64_t>(kCount));
    EXPECT_EQ(stats.freeCount, static_cast<std::uint64_t>(kCount));
}

TEST(Stress, AlternatingTierPressure) {
    ma::Allocator allocator;
    for (int i = 0; i < 2000; ++i) {
        void* small = allocator.allocate(64);
        void* medium = allocator.allocate(16 * 1024);
        void* large = allocator.allocate(512 * 1024);
        ASSERT_NE(small, nullptr);
        ASSERT_NE(medium, nullptr);
        ASSERT_NE(large, nullptr);
        allocator.deallocate(medium);
        allocator.deallocate(large);
        allocator.deallocate(small);
    }
    const auto stats = allocator.statistics();
    EXPECT_EQ(stats.activeAllocations, 0U);
}

} // namespace
