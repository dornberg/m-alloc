#include "MAlloc/MAlloc.hpp"

#include <cstdint>
#include <cstring>

#include <gtest/gtest.h>

namespace {

[[nodiscard]] bool isAligned(const void* ptr, std::size_t alignment) {
    return reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0;
}

TEST(Alignment, DefaultAllocationsAreSixteenByteAligned) {
    ma::Allocator allocator;
    for (std::size_t size : {1UL, 8UL, 100UL, 5000UL, 500000UL}) {
        void* ptr = allocator.allocate(size);
        ASSERT_NE(ptr, nullptr);
        EXPECT_TRUE(isAligned(ptr, 16));
        allocator.deallocate(ptr);
    }
}

TEST(Alignment, AlignedAllocateHonorsAlignment) {
    ma::Allocator allocator;
    for (std::size_t alignment : {32UL, 64UL, 128UL, 4096UL, 65536UL}) {
        void* ptr = allocator.alignedAllocate(alignment, 256);
        ASSERT_NE(ptr, nullptr);
        EXPECT_TRUE(isAligned(ptr, alignment));
        std::memset(ptr, 0x11, 256);
        allocator.deallocate(ptr);
    }
}

TEST(Alignment, AlignedAllocationSizeIsPreserved) {
    ma::Allocator allocator;
    void* ptr = allocator.alignedAllocate(256, 1000);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(allocator.allocationSize(ptr), 1000U);
    allocator.deallocate(ptr);
}

TEST(Alignment, NonPowerOfTwoAlignmentIsRejected) {
    ma::Allocator allocator;
    auto result = allocator.tryAlignedAllocate(48, 64);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ma::Error::InvalidAlignment);
}

TEST(Alignment, ZeroAlignmentIsRejected) {
    ma::Allocator allocator;
    auto result = allocator.tryAlignedAllocate(0, 64);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ma::Error::InvalidAlignment);
}

TEST(Alignment, SmallAlignmentFallsBackToRegularPath) {
    ma::Allocator allocator;
    void* ptr = allocator.alignedAllocate(8, 64);
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(isAligned(ptr, 8));
    allocator.deallocate(ptr);
}

} // namespace
