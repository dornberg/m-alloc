#pragma once

#include "Adapters.hpp"

#include <cstddef>
#include <cstdint>

namespace ma::bench {

struct BenchmarkResult {
    double allocateNanosPerOp = 0.0;
    double deallocateNanosPerOp = 0.0;
    double throughputMopsPerSec = 0.0;
    double multiThreadMopsPerSec = 0.0;
    std::uint64_t peakRssBytes = 0;
};

class Runner {
public:
    explicit Runner(std::size_t iterations) noexcept : iterations_(iterations) {}

    [[nodiscard]] BenchmarkResult run(AllocatorAdapter& adapter) const;

    [[nodiscard]] static std::uint64_t currentRss() noexcept;

private:
    void measureLatency(AllocatorAdapter& adapter, BenchmarkResult& result) const;
    void measureThroughput(AllocatorAdapter& adapter, BenchmarkResult& result) const;
    void measureMultiThreaded(AllocatorAdapter& adapter, BenchmarkResult& result) const;

    std::size_t iterations_;
};

} // namespace ma::bench
