#pragma once

#include "MAlloc/Config.hpp"
#include "MAlloc/Error.hpp"
#include "MAlloc/Statistics.hpp"

#include <cstddef>
#include <expected>
#include <memory>

namespace ma {

namespace detail {
class AllocatorImpl;
}

class Allocator {
public:
    Allocator();
    explicit Allocator(AllocatorConfig config);
    ~Allocator();

    Allocator(Allocator&& other) noexcept;
    Allocator& operator=(Allocator&& other) noexcept;
    Allocator(const Allocator&) = delete;
    Allocator& operator=(const Allocator&) = delete;

    [[nodiscard]] void* allocate(std::size_t size) noexcept;
    void deallocate(void* ptr) noexcept;
    [[nodiscard]] void* reallocate(void* ptr, std::size_t newSize) noexcept;
    [[nodiscard]] void* alignedAllocate(std::size_t alignment, std::size_t size) noexcept;

    [[nodiscard]] std::expected<void*, Error> tryAllocate(std::size_t size) noexcept;
    [[nodiscard]] std::expected<void, Error> tryDeallocate(void* ptr) noexcept;
    [[nodiscard]] std::expected<void*, Error> tryAlignedAllocate(std::size_t alignment,
                                                                 std::size_t size) noexcept;

    [[nodiscard]] std::size_t allocationSize(const void* ptr) const noexcept;
    [[nodiscard]] StatisticsSnapshot statistics() const noexcept;
    [[nodiscard]] const AllocatorConfig& config() const noexcept;
    [[nodiscard]] std::size_t liveAllocationCount() const noexcept;

private:
    std::unique_ptr<detail::AllocatorImpl> impl_;
};

} // namespace ma
