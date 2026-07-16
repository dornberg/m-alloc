#include "MAlloc/MAlloc.hpp"

#include <cstring>
#include <print>

int main() {
    ma::Allocator allocator;

    void* buffer = allocator.allocate(256);
    if (buffer == nullptr) {
        std::println(stderr, "allocation failed");
        return 1;
    }
    std::memset(buffer, 0, 256);
    std::println("allocated 256 bytes at {}", buffer);

    buffer = allocator.reallocate(buffer, 1024);
    std::println("grown to 1024 bytes at {}", buffer);

    void* aligned = allocator.alignedAllocate(64, 512);
    std::println("aligned block at {}", aligned);

    allocator.deallocate(aligned);
    allocator.deallocate(buffer);

    auto result = allocator.tryAllocate(128);
    if (result) {
        std::println("tryAllocate succeeded at {}", *result);
        allocator.deallocate(*result);
    } else {
        std::println(stderr, "tryAllocate failed: {}", ma::toString(result.error()));
    }
    return 0;
}
