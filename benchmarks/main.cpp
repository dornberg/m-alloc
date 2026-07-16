#include "MAlloc/MAlloc.hpp"

#include "Adapters.hpp"
#include "Runner.hpp"

#include <cstdlib>
#include <print>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    std::size_t iterations = 500000;
    if (argc > 1) {
        iterations = static_cast<std::size_t>(std::strtoull(argv[1], nullptr, 10));
        if (iterations == 0) {
            std::println(stderr, "usage: {} [iterations]", argv[0]);
            return 1;
        }
    }

    const ma::bench::Runner runner(iterations);
    auto adapters = ma::bench::makeAdapters();

    std::println("m-alloc benchmark suite ({} iterations per scenario)", iterations);
    std::println("{:<42} {:>12} {:>12} {:>12} {:>12} {:>12}",
                 "allocator",
                 "alloc ns/op",
                 "free ns/op",
                 "1T Mops/s",
                 "MT Mops/s",
                 "peak RSS MB");

    for (const auto& adapter : adapters) {
        const auto result = runner.run(*adapter);
        std::println("{:<42} {:>12.1f} {:>12.1f} {:>12.2f} {:>12.2f} {:>12.1f}",
                     adapter->name(),
                     result.allocateNanosPerOp,
                     result.deallocateNanosPerOp,
                     result.throughputMopsPerSec,
                     result.multiThreadMopsPerSec,
                     static_cast<double>(result.peakRssBytes) / (1024.0 * 1024.0));
    }

    ma::Allocator allocator;
    std::vector<void*> churn;
    for (int i = 0; i < 4000; ++i) {
        churn.push_back(allocator.allocate(static_cast<std::size_t>(i % 32 + 1) * 1024));
    }
    for (std::size_t i = 0; i < churn.size(); i += 2) {
        allocator.deallocate(churn[i]);
    }
    const auto stats = allocator.statistics();
    std::println("");
    std::println("m-alloc fragmentation under churn:");
    std::println("  internal fragmentation : {:.2f}%", stats.internalFragmentation * 100.0);
    std::println("  external fragmentation : {:.2f}%", stats.externalFragmentation * 100.0);
    for (std::size_t i = 1; i < churn.size(); i += 2) {
        allocator.deallocate(churn[i]);
    }
    return 0;
}
