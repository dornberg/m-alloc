#include "MAlloc/MAlloc.hpp"

#include <print>
#include <vector>

namespace {

void printStats(const ma::StatisticsSnapshot& stats) {
    std::println("  allocations          : {}", stats.allocationCount);
    std::println("  frees                : {}", stats.freeCount);
    std::println("  active allocations   : {}", stats.activeAllocations);
    std::println("  current usage        : {} bytes", stats.currentUsage);
    std::println("  peak usage           : {} bytes", stats.peakUsage);
    std::println("  allocated total      : {} bytes", stats.allocatedBytes);
    std::println("  freed total          : {} bytes", stats.freedBytes);
    std::println("  reserved             : {} bytes", stats.reservedBytes);
    std::println("  internal frag        : {:.2f}%", stats.internalFragmentation * 100.0);
    std::println("  external frag        : {:.2f}%", stats.externalFragmentation * 100.0);
}

} // namespace

int main() {
    ma::Allocator allocator;

    std::vector<void*> blocks;
    for (int i = 0; i < 500; ++i) {
        blocks.push_back(allocator.allocate(static_cast<std::size_t>(i % 1000) + 1));
    }

    std::println("after 500 allocations:");
    printStats(allocator.statistics());

    for (std::size_t i = 0; i < blocks.size(); i += 2) {
        allocator.deallocate(blocks[i]);
    }

    std::println("after freeing every second block:");
    printStats(allocator.statistics());

    for (std::size_t i = 1; i < blocks.size(); i += 2) {
        allocator.deallocate(blocks[i]);
    }

    std::println("after freeing everything:");
    printStats(allocator.statistics());
    return 0;
}
