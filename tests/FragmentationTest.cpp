#include "MAlloc/MAlloc.hpp"

#include <vector>

#include <gtest/gtest.h>

namespace {

TEST(Fragmentation, CoalescingAllowsLargeReallocationAfterFrees) {
    ma::AllocatorConfig config;
    config.enableThreadCache = false;
    ma::Allocator allocator(config);
    constexpr std::size_t kBlockSize = 32 * 1024;
    std::vector<void*> blocks;
    for (int i = 0; i < 64; ++i) {
        void* ptr = allocator.allocate(kBlockSize);
        ASSERT_NE(ptr, nullptr);
        blocks.push_back(ptr);
    }
    for (void* ptr : blocks) {
        allocator.deallocate(ptr);
    }
    void* big = allocator.allocate(200 * 1024);
    ASSERT_NE(big, nullptr);
    allocator.deallocate(big);
}

TEST(Fragmentation, ExternalFragmentationStaysInRange) {
    ma::AllocatorConfig config;
    config.enableThreadCache = false;
    ma::Allocator allocator(config);
    std::vector<void*> blocks;
    for (int i = 0; i < 128; ++i) {
        blocks.push_back(allocator.allocate(16 * 1024));
    }
    for (std::size_t i = 0; i < blocks.size(); i += 2) {
        allocator.deallocate(blocks[i]);
    }
    const auto stats = allocator.statistics();
    EXPECT_GE(stats.externalFragmentation, 0.0);
    EXPECT_LE(stats.externalFragmentation, 1.0);
    for (std::size_t i = 1; i < blocks.size(); i += 2) {
        allocator.deallocate(blocks[i]);
    }
}

TEST(Fragmentation, InterleavedFreePatternRemainsUsable) {
    ma::AllocatorConfig config;
    config.enableThreadCache = false;
    ma::Allocator allocator(config);
    std::vector<void*> blocks;
    for (int i = 0; i < 256; ++i) {
        void* ptr = allocator.allocate(8 * 1024);
        ASSERT_NE(ptr, nullptr);
        blocks.push_back(ptr);
    }
    for (std::size_t i = 0; i < blocks.size(); i += 2) {
        allocator.deallocate(blocks[i]);
        blocks[i] = allocator.allocate(12 * 1024);
        ASSERT_NE(blocks[i], nullptr);
    }
    for (void* ptr : blocks) {
        allocator.deallocate(ptr);
    }
    const auto stats = allocator.statistics();
    EXPECT_EQ(stats.activeAllocations, 0U);
}

} // namespace
