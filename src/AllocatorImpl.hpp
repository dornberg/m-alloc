#pragma once

#include "MAlloc/Config.hpp"
#include "MAlloc/Error.hpp"
#include "MAlloc/Statistics.hpp"

#include "BlockHeader.hpp"
#include "CentralState.hpp"
#include "DebugLayer.hpp"
#include "ThreadCache.hpp"

#include <cstddef>
#include <expected>
#include <memory>

namespace ma::detail {

class AllocatorImpl {
public:
    explicit AllocatorImpl(AllocatorConfig config);
    ~AllocatorImpl();

    AllocatorImpl(const AllocatorImpl&) = delete;
    AllocatorImpl& operator=(const AllocatorImpl&) = delete;
    AllocatorImpl(AllocatorImpl&&) = delete;
    AllocatorImpl& operator=(AllocatorImpl&&) = delete;

    [[nodiscard]] std::expected<void*, Error> allocate(std::size_t size) noexcept;
    [[nodiscard]] std::expected<void, Error> deallocate(void* ptr) noexcept;
    [[nodiscard]] std::expected<void*, Error> reallocate(void* ptr, std::size_t newSize) noexcept;
    [[nodiscard]] std::expected<void*, Error> alignedAllocate(std::size_t alignment,
                                                              std::size_t size) noexcept;

    [[nodiscard]] std::size_t allocationSize(const void* ptr) const noexcept;
    [[nodiscard]] StatisticsSnapshot statistics() const noexcept;

    [[nodiscard]] const AllocatorConfig& config() const noexcept { return central_->config; }

    [[nodiscard]] std::size_t liveAllocationCount() const noexcept { return debug_.liveCount(); }

private:
    struct RawAllocation {
        void* user;
        std::size_t reserved;
    };

    static constexpr std::size_t kMaxRequest = std::size_t{1} << 46;
    static constexpr std::size_t kMaxAlignment = std::size_t{1} << 30;

    [[nodiscard]] std::expected<RawAllocation, Error> rawAllocate(std::size_t size) noexcept;
    [[nodiscard]] std::expected<std::size_t, Error> rawDeallocate(void* raw) noexcept;
    [[nodiscard]] ThreadCache* threadCache() noexcept;
    void report(Error error, const void* ptr) const noexcept;

    std::shared_ptr<CentralState> central_;
    DebugLayer debug_;
};

} // namespace ma::detail
