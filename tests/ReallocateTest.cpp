#include "MAlloc/MAlloc.hpp"

#include <cstdint>
#include <cstring>

#include <gtest/gtest.h>

namespace {

TEST(Reallocate, NullPointerBehavesLikeAllocate) {
    ma::Allocator allocator;
    void* ptr = allocator.reallocate(nullptr, 128);
    ASSERT_NE(ptr, nullptr);
    allocator.deallocate(ptr);
}

TEST(Reallocate, GrowPreservesContents) {
    ma::Allocator allocator;
    auto* ptr = static_cast<std::uint8_t*>(allocator.allocate(64));
    ASSERT_NE(ptr, nullptr);
    for (std::uint8_t i = 0; i < 64; ++i) {
        ptr[i] = i;
    }
    auto* grown = static_cast<std::uint8_t*>(allocator.reallocate(ptr, 8192));
    ASSERT_NE(grown, nullptr);
    for (std::uint8_t i = 0; i < 64; ++i) {
        ASSERT_EQ(grown[i], i);
    }
    allocator.deallocate(grown);
}

TEST(Reallocate, ShrinkPreservesPrefix) {
    ma::Allocator allocator;
    auto* ptr = static_cast<std::uint8_t*>(allocator.allocate(1024));
    ASSERT_NE(ptr, nullptr);
    std::memset(ptr, 0x7E, 1024);
    auto* shrunk = static_cast<std::uint8_t*>(allocator.reallocate(ptr, 16));
    ASSERT_NE(shrunk, nullptr);
    for (std::size_t i = 0; i < 16; ++i) {
        ASSERT_EQ(shrunk[i], 0x7E);
    }
    allocator.deallocate(shrunk);
}

TEST(Reallocate, ZeroSizeFreesPointer) {
    ma::Allocator allocator;
    void* ptr = allocator.allocate(256);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(allocator.reallocate(ptr, 0), nullptr);
    const auto stats = allocator.statistics();
    EXPECT_EQ(stats.activeAllocations, 0U);
}

TEST(Reallocate, CrossTierGrowth) {
    ma::Allocator allocator;
    auto* ptr = static_cast<std::uint8_t*>(allocator.allocate(100));
    ASSERT_NE(ptr, nullptr);
    std::memset(ptr, 0x33, 100);
    auto* huge = static_cast<std::uint8_t*>(allocator.reallocate(ptr, 2 * 1024 * 1024));
    ASSERT_NE(huge, nullptr);
    for (std::size_t i = 0; i < 100; ++i) {
        ASSERT_EQ(huge[i], 0x33);
    }
    allocator.deallocate(huge);
}

} // namespace
