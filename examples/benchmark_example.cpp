#include "MAlloc/MAlloc.hpp"

#include <chrono>
#include <cstdlib>
#include <print>
#include <vector>

namespace {

template <typename AllocateFn, typename FreeFn>
double measure(AllocateFn allocate, FreeFn release, int iterations) {
    std::vector<void*> pointers;
    pointers.reserve(static_cast<std::size_t>(iterations));
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        pointers.push_back(allocate(static_cast<std::size_t>(i % 256) + 16));
    }
    for (void* ptr : pointers) {
        release(ptr);
    }
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

} // namespace

int main() {
    constexpr int kIterations = 200000;
    ma::Allocator allocator;

    const double mallocTime = measure([](std::size_t size) { return std::malloc(size); },
                                      [](void* ptr) { std::free(ptr); },
                                      kIterations);

    const double mallocLibTime =
        measure([&allocator](std::size_t size) { return allocator.allocate(size); },
                [&allocator](void* ptr) { allocator.deallocate(ptr); },
                kIterations);

    std::println("{} small allocations + frees", kIterations);
    std::println("std::malloc : {:.2f} ms", mallocTime);
    std::println("m-alloc     : {:.2f} ms", mallocLibTime);
    return 0;
}
