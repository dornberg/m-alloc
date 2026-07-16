#include "MAlloc/MAlloc.hpp"

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

namespace {

struct ErrorRecorder {
    std::vector<ma::Error> errors;

    [[nodiscard]] ma::AllocatorConfig makeConfig() {
        ma::AllocatorConfig config = ma::AllocatorConfig::debug();
        config.errorHandler = [this](ma::Error error, const void*) { errors.push_back(error); };
        return config;
    }
};

TEST(DebugFeatures, DoubleFreeIsDetected) {
    ErrorRecorder recorder;
    ma::Allocator allocator(recorder.makeConfig());
    void* ptr = allocator.allocate(64);
    ASSERT_NE(ptr, nullptr);
    allocator.deallocate(ptr);
    auto result = allocator.tryDeallocate(ptr);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ma::Error::DoubleFree);
    ASSERT_EQ(recorder.errors.size(), 1U);
    EXPECT_EQ(recorder.errors.front(), ma::Error::DoubleFree);
}

TEST(DebugFeatures, InvalidPointerIsDetected) {
    ErrorRecorder recorder;
    ma::Allocator allocator(recorder.makeConfig());
    std::uint64_t local = 0;
    auto result = allocator.tryDeallocate(&local);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ma::Error::InvalidPointer);
}

TEST(DebugFeatures, DoubleFreeWithoutTrackingIsDetected) {
    ma::AllocatorConfig config;
    config.enableThreadCache = false;
    std::vector<ma::Error> errors;
    config.errorHandler = [&errors](ma::Error error, const void*) { errors.push_back(error); };
    ma::Allocator allocator(config);
    void* ptr = allocator.allocate(64);
    ASSERT_NE(ptr, nullptr);
    allocator.deallocate(ptr);
    auto result = allocator.tryDeallocate(ptr);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ma::Error::DoubleFree);
}

TEST(DebugFeatures, GuardCorruptionIsDetected) {
    ErrorRecorder recorder;
    ma::Allocator allocator(recorder.makeConfig());
    auto* ptr = static_cast<std::uint8_t*>(allocator.allocate(32));
    ASSERT_NE(ptr, nullptr);
    ptr[32] = 0xFF;
    allocator.deallocate(ptr);
    ASSERT_FALSE(recorder.errors.empty());
    EXPECT_EQ(recorder.errors.front(), ma::Error::HeapCorruption);
}

TEST(DebugFeatures, PoisonPatternIsAppliedOnAllocation) {
    ma::AllocatorConfig config;
    config.enablePoisoning = true;
    config.enableThreadCache = false;
    ma::Allocator allocator(config);
    auto* ptr = static_cast<std::uint8_t*>(allocator.allocate(64));
    ASSERT_NE(ptr, nullptr);
    for (std::size_t i = 0; i < 64; ++i) {
        ASSERT_EQ(ptr[i], 0xCD);
    }
    allocator.deallocate(ptr);
}

TEST(DebugFeatures, LeakTrackingCountsLiveAllocations) {
    ma::Allocator allocator(ma::AllocatorConfig::debug());
    EXPECT_EQ(allocator.liveAllocationCount(), 0U);
    void* first = allocator.allocate(32);
    void* second = allocator.allocate(64);
    EXPECT_EQ(allocator.liveAllocationCount(), 2U);
    allocator.deallocate(first);
    EXPECT_EQ(allocator.liveAllocationCount(), 1U);
    allocator.deallocate(second);
    EXPECT_EQ(allocator.liveAllocationCount(), 0U);
}

TEST(DebugFeatures, NullDeallocateIsNoOp) {
    ErrorRecorder recorder;
    ma::Allocator allocator(recorder.makeConfig());
    allocator.deallocate(nullptr);
    EXPECT_TRUE(recorder.errors.empty());
}

} // namespace
