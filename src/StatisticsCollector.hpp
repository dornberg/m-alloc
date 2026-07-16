#pragma once

#include "MAlloc/Statistics.hpp"

#include <atomic>
#include <cstdint>

namespace ma::detail {

class StatisticsCollector {
public:
    void recordAllocation(std::uint64_t requested, std::uint64_t reserved) noexcept {
        allocationCount_.fetch_add(1, std::memory_order_relaxed);
        allocatedBytes_.fetch_add(requested, std::memory_order_relaxed);
        liveRequested_.fetch_add(requested, std::memory_order_relaxed);
        liveReserved_.fetch_add(reserved, std::memory_order_relaxed);
        activeAllocations_.fetch_add(1, std::memory_order_relaxed);
        updatePeak(liveRequested_.load(std::memory_order_relaxed));
    }

    void recordFree(std::uint64_t requested, std::uint64_t reserved) noexcept {
        freeCount_.fetch_add(1, std::memory_order_relaxed);
        freedBytes_.fetch_add(requested, std::memory_order_relaxed);
        liveRequested_.fetch_sub(requested, std::memory_order_relaxed);
        liveReserved_.fetch_sub(reserved, std::memory_order_relaxed);
        activeAllocations_.fetch_sub(1, std::memory_order_relaxed);
    }

    [[nodiscard]] StatisticsSnapshot snapshot(double externalFragmentation) const noexcept {
        StatisticsSnapshot result{};
        result.allocationCount = allocationCount_.load(std::memory_order_relaxed);
        result.freeCount = freeCount_.load(std::memory_order_relaxed);
        result.activeAllocations = activeAllocations_.load(std::memory_order_relaxed);
        result.currentUsage = liveRequested_.load(std::memory_order_relaxed);
        result.peakUsage = peakUsage_.load(std::memory_order_relaxed);
        result.allocatedBytes = allocatedBytes_.load(std::memory_order_relaxed);
        result.freedBytes = freedBytes_.load(std::memory_order_relaxed);
        result.reservedBytes = liveReserved_.load(std::memory_order_relaxed);
        const auto reserved = static_cast<double>(result.reservedBytes);
        const auto requested = static_cast<double>(result.currentUsage);
        result.internalFragmentation = reserved > 0.0 ? (reserved - requested) / reserved : 0.0;
        result.externalFragmentation = externalFragmentation;
        return result;
    }

private:
    void updatePeak(std::uint64_t current) noexcept {
        std::uint64_t peak = peakUsage_.load(std::memory_order_relaxed);
        while (current > peak &&
               !peakUsage_.compare_exchange_weak(peak, current, std::memory_order_relaxed)) {
        }
    }

    std::atomic<std::uint64_t> allocationCount_{0};
    std::atomic<std::uint64_t> freeCount_{0};
    std::atomic<std::uint64_t> activeAllocations_{0};
    std::atomic<std::uint64_t> liveRequested_{0};
    std::atomic<std::uint64_t> liveReserved_{0};
    std::atomic<std::uint64_t> peakUsage_{0};
    std::atomic<std::uint64_t> allocatedBytes_{0};
    std::atomic<std::uint64_t> freedBytes_{0};
};

} // namespace ma::detail
