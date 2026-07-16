#pragma once

#include "SizeClassTable.hpp"

#include <array>
#include <cstddef>
#include <memory>

namespace ma::detail {

struct CentralState;
class PoolAllocator;

class ThreadCache {
public:
    explicit ThreadCache(std::weak_ptr<CentralState> central) noexcept
        : central_(std::move(central)) {}

    ~ThreadCache();

    ThreadCache(const ThreadCache&) = delete;
    ThreadCache& operator=(const ThreadCache&) = delete;
    ThreadCache(ThreadCache&&) = delete;
    ThreadCache& operator=(ThreadCache&&) = delete;

    [[nodiscard]] void* allocateBlock(std::size_t classIndex, PoolAllocator& pools) noexcept;
    void freeBlock(std::size_t classIndex, void* block, PoolAllocator& pools) noexcept;

    [[nodiscard]] bool expired() const noexcept { return central_.expired(); }

private:
    static constexpr std::size_t kCapacity = 64;
    static constexpr std::size_t kBatchSize = 16;

    struct ClassCache {
        std::array<void*, kCapacity> slots{};
        std::size_t count = 0;
    };

    std::weak_ptr<CentralState> central_;
    std::array<ClassCache, SizeClassTable::kClassCount> caches_{};
};

} // namespace ma::detail
