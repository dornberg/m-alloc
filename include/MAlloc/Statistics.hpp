#pragma once

#include <cstdint>

namespace ma {

struct StatisticsSnapshot {
    std::uint64_t allocationCount = 0;
    std::uint64_t freeCount = 0;
    std::uint64_t activeAllocations = 0;
    std::uint64_t currentUsage = 0;
    std::uint64_t peakUsage = 0;
    std::uint64_t allocatedBytes = 0;
    std::uint64_t freedBytes = 0;
    std::uint64_t reservedBytes = 0;
    double internalFragmentation = 0.0;
    double externalFragmentation = 0.0;
};

} // namespace ma
