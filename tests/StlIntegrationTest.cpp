#include "MAlloc/MAlloc.hpp"

#include <deque>
#include <map>
#include <numeric>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace {

TEST(StlIntegration, VectorWithStlAllocator) {
    ma::Allocator allocator;
    std::vector<int, ma::StlAllocator<int>> values{ma::StlAllocator<int>(allocator)};
    for (int i = 0; i < 10000; ++i) {
        values.push_back(i);
    }
    EXPECT_EQ(std::accumulate(values.begin(), values.end(), 0LL), 49995000LL);
}

TEST(StlIntegration, MapWithStlAllocator) {
    ma::Allocator allocator;
    using Alloc = ma::StlAllocator<std::pair<const int, std::string>>;
    std::map<int, std::string, std::less<>, Alloc> table{Alloc(allocator)};
    for (int i = 0; i < 100; ++i) {
        table.emplace(i, std::to_string(i));
    }
    EXPECT_EQ(table.size(), 100U);
    EXPECT_EQ(table.at(42), "42");
}

TEST(StlIntegration, DequeChurn) {
    ma::Allocator allocator;
    std::deque<double, ma::StlAllocator<double>> values{ma::StlAllocator<double>(allocator)};
    for (int i = 0; i < 5000; ++i) {
        values.push_back(i * 0.5);
        if (i % 3 == 0 && !values.empty()) {
            values.pop_front();
        }
    }
    EXPECT_FALSE(values.empty());
}

TEST(StlIntegration, PmrMemoryResource) {
    ma::Allocator allocator;
    ma::MemoryResource resource(allocator);
    std::pmr::vector<std::pmr::string> names{&resource};
    for (int i = 0; i < 1000; ++i) {
        names.emplace_back("name-" + std::to_string(i));
    }
    EXPECT_EQ(names.size(), 1000U);
    EXPECT_EQ(names.front(), "name-0");
}

TEST(StlIntegration, AllocationsAreReleasedOnScopeExit) {
    ma::Allocator allocator;
    {
        std::vector<int, ma::StlAllocator<int>> values{ma::StlAllocator<int>(allocator)};
        values.resize(4096);
    }
    const auto stats = allocator.statistics();
    EXPECT_EQ(stats.activeAllocations, 0U);
}

} // namespace
