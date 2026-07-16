#include "PoolAllocator.hpp"

namespace ma::detail {

void* PoolAllocator::allocateBlock(std::size_t classIndex) noexcept {
    Shard& shard = shards_[classIndex];
    const std::lock_guard lock(shard.mutex);
    return takeBlock(shard, blockStride(classIndex));
}

void PoolAllocator::freeBlock(std::size_t classIndex, void* block) noexcept {
    Shard& shard = shards_[classIndex];
    const std::lock_guard lock(shard.mutex);
    shard.freeList.push(block);
}

std::size_t PoolAllocator::allocateBatch(std::size_t classIndex, std::span<void*> out) noexcept {
    Shard& shard = shards_[classIndex];
    const std::lock_guard lock(shard.mutex);
    const std::size_t stride = blockStride(classIndex);
    std::size_t count = 0;
    while (count < out.size()) {
        void* block = takeBlock(shard, stride);
        if (block == nullptr) {
            break;
        }
        out[count] = block;
        ++count;
    }
    return count;
}

void PoolAllocator::freeBatch(std::size_t classIndex, std::span<void* const> blocks) noexcept {
    Shard& shard = shards_[classIndex];
    const std::lock_guard lock(shard.mutex);
    for (void* block : blocks) {
        shard.freeList.push(block);
    }
}

bool PoolAllocator::refill(Shard& shard) noexcept {
    MappedRegion run(kRunSize);
    if (!run.valid()) {
        return false;
    }
    shard.bumpCursor = run.data();
    shard.bumpEnd = run.data() + run.size();
    shard.runs.push_back(std::move(run));
    return true;
}

void* PoolAllocator::takeBlock(Shard& shard, std::size_t stride) noexcept {
    if (void* block = shard.freeList.pop(); block != nullptr) {
        return block;
    }
    if (shard.bumpCursor == nullptr ||
        static_cast<std::size_t>(shard.bumpEnd - shard.bumpCursor) < stride) {
        if (!refill(shard)) {
            return nullptr;
        }
    }
    std::byte* block = shard.bumpCursor;
    shard.bumpCursor += stride;
    return block;
}

} // namespace ma::detail
