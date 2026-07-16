#pragma once

#include "BlockHeader.hpp"
#include "FreeList.hpp"
#include "SizeClassTable.hpp"
#include "SystemMemory.hpp"

#include <array>
#include <cstddef>
#include <mutex>
#include <span>
#include <vector>

namespace ma::detail {

class PoolAllocator {
public:
    PoolAllocator() = default;
    ~PoolAllocator() = default;

    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;
    PoolAllocator(PoolAllocator&&) = delete;
    PoolAllocator& operator=(PoolAllocator&&) = delete;

    [[nodiscard]] void* allocateBlock(std::size_t classIndex) noexcept;
    void freeBlock(std::size_t classIndex, void* block) noexcept;
    [[nodiscard]] std::size_t allocateBatch(std::size_t classIndex, std::span<void*> out) noexcept;
    void freeBatch(std::size_t classIndex, std::span<void* const> blocks) noexcept;

    [[nodiscard]] static constexpr std::size_t blockStride(std::size_t classIndex) noexcept {
        return alignUp(kHeaderSize + SizeClassTable::classSize(classIndex), kDefaultAlignment);
    }

private:
    struct Shard {
        std::mutex mutex;
        FreeList freeList;
        std::byte* bumpCursor = nullptr;
        std::byte* bumpEnd = nullptr;
        std::vector<MappedRegion> runs;
    };

    static constexpr std::size_t kRunSize = 64 * 1024;

    [[nodiscard]] static bool refill(Shard& shard) noexcept;
    [[nodiscard]] static void* takeBlock(Shard& shard, std::size_t stride) noexcept;

    std::array<Shard, SizeClassTable::kClassCount> shards_{};
};

} // namespace ma::detail
