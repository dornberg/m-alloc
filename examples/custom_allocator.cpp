#include "MAlloc/MAlloc.hpp"

#include <print>

int main() {
    ma::AllocatorConfig config = ma::AllocatorConfig::debug();
    config.errorHandler = [](ma::Error error, const void* ptr) {
        std::println(stderr, "allocator error: {} at {}", ma::toString(error), ptr);
    };

    ma::Allocator allocator(config);

    void* block = allocator.allocate(128);
    std::println("debug-mode block at {}", block);

    allocator.deallocate(block);
    allocator.deallocate(block);

    int stackValue = 0;
    allocator.deallocate(&stackValue);

    std::println("live allocations: {}", allocator.liveAllocationCount());
    return 0;
}
