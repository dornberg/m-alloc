#include "MAlloc/MAlloc.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>

namespace {

TEST(LargeAllocation, MediumTierAllocates) {
    ma::Allocator allocator;
    for (std::size_t size : {8UL * 1024, 64UL * 1024, 200UL * 1024}) {
        auto* ptr = static_cast<std::uint8_t*>(allocator.allocate(size));
        ASSERT_NE(ptr, nullptr);
        std::memset(ptr, 0x5A, size);
        EXPECT_EQ(ptr[size - 1], 0x5A);
        allocator.deallocate(ptr);
    }
}

TEST(LargeAllocation, LargeTierAllocates) {
    ma::Allocator allocator;
    for (std::size_t size : {512UL * 1024, 4UL * 1024 * 1024, 32UL * 1024 * 1024}) {
        auto* ptr = static_cast<std::uint8_t*>(allocator.allocate(size));
        ASSERT_NE(ptr, nullptr);
        ptr[0] = 1;
        ptr[size - 1] = 2;
        EXPECT_EQ(ptr[0], 1);
        EXPECT_EQ(ptr[size - 1], 2);
        allocator.deallocate(ptr);
    }
}

TEST(LargeAllocation, SizeOverflowIsRejected) {
    ma::Allocator allocator;
    auto result = allocator.tryAllocate(std::size_t{1} << 60);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ma::Error::SizeOverflow);
}

TEST(LargeAllocation, AllocationSizeMatchesRequest) {
    ma::Allocator allocator;
    const std::size_t size = 3 * 1024 * 1024;
    void* ptr = allocator.allocate(size);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(allocator.allocationSize(ptr), size);
    allocator.deallocate(ptr);
}

TEST(LargeAllocation, ManyLargeBlocksCoexist) {
    ma::Allocator allocator;
    constexpr std::size_t kCount = 16;
    constexpr std::size_t kSize = 1024 * 1024;
    std::array<void*, kCount> blocks{};
    for (auto& block : blocks) {
        block = allocator.allocate(kSize);
        ASSERT_NE(block, nullptr);
    }
    for (void* block : blocks) {
        allocator.deallocate(block);
    }
}

} // namespace
