#include "MAlloc/MAlloc.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <random>
#include <vector>

namespace {

struct Record {
    void* ptr;
    std::size_t size;
    std::uint8_t fill;
};

TEST(RandomWorkload, MixedSizesSurviveChurn) {
    ma::Allocator allocator;
    std::mt19937 rng(42);
    std::uniform_int_distribution<std::size_t> sizeDist(1, 512 * 1024);
    std::uniform_int_distribution<int> actionDist(0, 99);
    std::vector<Record> live;
    for (int iteration = 0; iteration < 20000; ++iteration) {
        const bool doAllocate = live.empty() || actionDist(rng) < 60;
        if (doAllocate) {
            const std::size_t size = sizeDist(rng);
            auto* ptr = static_cast<std::uint8_t*>(allocator.allocate(size));
            ASSERT_NE(ptr, nullptr);
            const auto fill = static_cast<std::uint8_t>(iteration & 0xFF);
            ptr[0] = fill;
            ptr[size - 1] = fill;
            live.push_back({ptr, size, fill});
        } else {
            std::uniform_int_distribution<std::size_t> indexDist(0, live.size() - 1);
            const std::size_t index = indexDist(rng);
            const Record record = live[index];
            auto* bytes = static_cast<std::uint8_t*>(record.ptr);
            ASSERT_EQ(bytes[0], record.fill);
            ASSERT_EQ(bytes[record.size - 1], record.fill);
            allocator.deallocate(record.ptr);
            live[index] = live.back();
            live.pop_back();
        }
    }
    for (const Record& record : live) {
        allocator.deallocate(record.ptr);
    }
    const auto stats = allocator.statistics();
    EXPECT_EQ(stats.activeAllocations, 0U);
    EXPECT_EQ(stats.currentUsage, 0U);
}

TEST(RandomWorkload, RandomReallocationsPreserveData) {
    ma::Allocator allocator;
    std::mt19937 rng(7);
    std::uniform_int_distribution<std::size_t> sizeDist(1, 64 * 1024);
    std::size_t size = sizeDist(rng);
    auto* ptr = static_cast<std::uint8_t*>(allocator.allocate(size));
    ASSERT_NE(ptr, nullptr);
    std::memset(ptr, 0x42, size);
    for (int i = 0; i < 200; ++i) {
        const std::size_t newSize = sizeDist(rng);
        auto* resized = static_cast<std::uint8_t*>(allocator.reallocate(ptr, newSize));
        ASSERT_NE(resized, nullptr);
        const std::size_t checked = std::min(size, newSize);
        for (std::size_t j = 0; j < checked; j += 977) {
            ASSERT_EQ(resized[j], 0x42);
        }
        std::memset(resized, 0x42, newSize);
        ptr = resized;
        size = newSize;
    }
    allocator.deallocate(ptr);
}

} // namespace
