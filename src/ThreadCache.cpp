#include "ThreadCache.hpp"

#include "CentralState.hpp"
#include "PoolAllocator.hpp"

#include <span>

namespace ma::detail {

ThreadCache::~ThreadCache() {
    const auto central = central_.lock();
    if (!central) {
        return;
    }
    for (std::size_t classIndex = 0; classIndex < caches_.size(); ++classIndex) {
        ClassCache& cache = caches_[classIndex];
        if (cache.count > 0) {
            central->pools.freeBatch(classIndex, std::span(cache.slots.data(), cache.count));
            cache.count = 0;
        }
    }
}

void* ThreadCache::allocateBlock(std::size_t classIndex, PoolAllocator& pools) noexcept {
    ClassCache& cache = caches_[classIndex];
    if (cache.count == 0) {
        cache.count = pools.allocateBatch(classIndex, std::span(cache.slots.data(), kBatchSize));
        if (cache.count == 0) {
            return nullptr;
        }
    }
    --cache.count;
    return cache.slots[cache.count];
}

void ThreadCache::freeBlock(std::size_t classIndex, void* block, PoolAllocator& pools) noexcept {
    ClassCache& cache = caches_[classIndex];
    if (cache.count == kCapacity) {
        const std::size_t keep = kCapacity - kBatchSize;
        pools.freeBatch(classIndex, std::span(cache.slots.data() + keep, kBatchSize));
        cache.count = keep;
    }
    cache.slots[cache.count] = block;
    ++cache.count;
}

} // namespace ma::detail
