#include "MAlloc/MAlloc.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <random>
#include <thread>
#include <vector>

namespace {

TEST(ThreadSafety, ConcurrentAllocateFree) {
    ma::Allocator allocator;
    constexpr int kThreads = 8;
    constexpr int kIterations = 5000;
    std::atomic<bool> failed{false};
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&allocator, &failed, t] {
            std::mt19937 rng(static_cast<std::mt19937::result_type>(t));
            std::uniform_int_distribution<std::size_t> sizeDist(1, 8192);
            std::vector<void*> local;
            local.reserve(64);
            for (int i = 0; i < kIterations; ++i) {
                void* ptr = allocator.allocate(sizeDist(rng));
                if (ptr == nullptr) {
                    failed.store(true);
                    return;
                }
                local.push_back(ptr);
                if (local.size() >= 64) {
                    for (void* p : local) {
                        allocator.deallocate(p);
                    }
                    local.clear();
                }
            }
            for (void* p : local) {
                allocator.deallocate(p);
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    EXPECT_FALSE(failed.load());
    const auto stats = allocator.statistics();
    EXPECT_EQ(stats.activeAllocations, 0U);
    EXPECT_EQ(stats.allocationCount, static_cast<std::uint64_t>(kThreads) * kIterations);
}

TEST(ThreadSafety, CrossThreadFree) {
    ma::Allocator allocator;
    constexpr int kCount = 20000;
    std::vector<void*> pointers(kCount);
    std::thread producer([&allocator, &pointers] {
        for (auto& slot : pointers) {
            slot = allocator.allocate(128);
        }
    });
    producer.join();
    std::thread consumer([&allocator, &pointers] {
        for (void* ptr : pointers) {
            ASSERT_NE(ptr, nullptr);
            allocator.deallocate(ptr);
        }
    });
    consumer.join();
    const auto stats = allocator.statistics();
    EXPECT_EQ(stats.activeAllocations, 0U);
}

TEST(ThreadSafety, ThreadCacheDisabledStillSafe) {
    ma::AllocatorConfig config;
    config.enableThreadCache = false;
    ma::Allocator allocator(config);
    constexpr int kThreads = 4;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&allocator] {
            for (int i = 0; i < 2000; ++i) {
                void* ptr = allocator.allocate(static_cast<std::size_t>(i % 512) + 1);
                ASSERT_NE(ptr, nullptr);
                allocator.deallocate(ptr);
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    const auto stats = allocator.statistics();
    EXPECT_EQ(stats.activeAllocations, 0U);
}

} // namespace
