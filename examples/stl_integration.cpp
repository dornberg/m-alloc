#include "MAlloc/MAlloc.hpp"

#include <map>
#include <print>
#include <string>
#include <vector>

int main() {
    ma::Allocator allocator;

    std::vector<int, ma::StlAllocator<int>> numbers{ma::StlAllocator<int>(allocator)};
    for (int i = 0; i < 1000; ++i) {
        numbers.push_back(i * i);
    }
    std::println("vector holds {} squares, last = {}", numbers.size(), numbers.back());

    ma::MemoryResource resource(allocator);
    std::pmr::map<int, std::pmr::string> registry{&resource};
    registry.emplace(1, "alpha");
    registry.emplace(2, "beta");
    std::println("pmr map entry 2 = {}", registry.at(2));

    const auto stats = allocator.statistics();
    std::println("live allocations backing containers: {}", stats.activeAllocations);
    return 0;
}
