#include "Runner.hpp"

#include <algorithm>
#include <chrono>
#include <random>
#include <thread>
#include <vector>

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <psapi.h>
    #include <windows.h>
#else
    #include <sys/resource.h>
#endif

namespace ma::bench {

namespace {

using Clock = std::chrono::steady_clock;

[[nodiscard]] double nanosSince(Clock::time_point start, std::size_t operations) {
    const auto elapsed = std::chrono::duration<double, std::nano>(Clock::now() - start).count();
    return elapsed / static_cast<double>(operations);
}

} // namespace

BenchmarkResult Runner::run(AllocatorAdapter& adapter) const {
    BenchmarkResult result;
    measureLatency(adapter, result);
    measureThroughput(adapter, result);
    measureMultiThreaded(adapter, result);
    result.peakRssBytes = currentRss();
    return result;
}

void Runner::measureLatency(AllocatorAdapter& adapter, BenchmarkResult& result) const {
    std::vector<void*> pointers(iterations_);
    const auto allocStart = Clock::now();
    for (std::size_t i = 0; i < iterations_; ++i) {
        pointers[i] = adapter.allocate(64);
    }
    result.allocateNanosPerOp = nanosSince(allocStart, iterations_);
    const auto freeStart = Clock::now();
    for (std::size_t i = 0; i < iterations_; ++i) {
        adapter.deallocate(pointers[i], 64);
    }
    result.deallocateNanosPerOp = nanosSince(freeStart, iterations_);
}

void Runner::measureThroughput(AllocatorAdapter& adapter, BenchmarkResult& result) const {
    std::mt19937 rng(1234);
    std::uniform_int_distribution<std::size_t> sizeDist(16, 8192);
    std::vector<std::pair<void*, std::size_t>> live;
    live.reserve(1024);
    const auto start = Clock::now();
    std::size_t operations = 0;
    for (std::size_t i = 0; i < iterations_; ++i) {
        const std::size_t size = sizeDist(rng);
        live.emplace_back(adapter.allocate(size), size);
        ++operations;
        if (live.size() >= 1024) {
            for (const auto& [ptr, blockSize] : live) {
                adapter.deallocate(ptr, blockSize);
                ++operations;
            }
            live.clear();
        }
    }
    for (const auto& [ptr, blockSize] : live) {
        adapter.deallocate(ptr, blockSize);
        ++operations;
    }
    const auto elapsed = std::chrono::duration<double>(Clock::now() - start).count();
    result.throughputMopsPerSec = static_cast<double>(operations) / elapsed / 1e6;
}

void Runner::measureMultiThreaded(AllocatorAdapter& adapter, BenchmarkResult& result) const {
    if (!adapter.threadSafe()) {
        return;
    }
    const unsigned threadCount = std::max(2U, std::thread::hardware_concurrency() / 2);
    const std::size_t perThread = iterations_ / threadCount;
    const auto start = Clock::now();
    std::vector<std::thread> threads;
    threads.reserve(threadCount);
    for (unsigned t = 0; t < threadCount; ++t) {
        threads.emplace_back([&adapter, perThread, t] {
            std::mt19937 rng(t + 99);
            std::uniform_int_distribution<std::size_t> sizeDist(16, 2048);
            std::vector<std::pair<void*, std::size_t>> live;
            live.reserve(256);
            for (std::size_t i = 0; i < perThread; ++i) {
                const std::size_t size = sizeDist(rng);
                live.emplace_back(adapter.allocate(size), size);
                if (live.size() >= 256) {
                    for (const auto& [ptr, blockSize] : live) {
                        adapter.deallocate(ptr, blockSize);
                    }
                    live.clear();
                }
            }
            for (const auto& [ptr, blockSize] : live) {
                adapter.deallocate(ptr, blockSize);
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    const auto elapsed = std::chrono::duration<double>(Clock::now() - start).count();
    const auto operations = static_cast<double>(perThread) * threadCount * 2;
    result.multiThreadMopsPerSec = operations / elapsed / 1e6;
}

std::uint64_t Runner::currentRss() noexcept {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS counters{};
    if (::GetProcessMemoryInfo(::GetCurrentProcess(), &counters, sizeof(counters)) == 0) {
        return 0;
    }
    return counters.PeakWorkingSetSize;
#else
    rusage usage{};
    if (::getrusage(RUSAGE_SELF, &usage) != 0) {
        return 0;
    }
    return static_cast<std::uint64_t>(usage.ru_maxrss) * 1024;
#endif
}

} // namespace ma::bench
