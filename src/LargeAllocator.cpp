#include "LargeAllocator.hpp"

namespace ma::detail {

void* LargeAllocator::allocate(std::size_t bytes) noexcept {
    MappedRegion region(bytes + kPayloadOffset);
    if (!region.valid()) {
        return nullptr;
    }
    std::byte* payload = region.data() + kPayloadOffset;
    const std::lock_guard lock(mutex_);
    regions_.emplace(payload, std::move(region));
    return payload;
}

bool LargeAllocator::deallocate(void* payload) noexcept {
    const std::lock_guard lock(mutex_);
    return regions_.erase(payload) > 0;
}

bool LargeAllocator::contains(const void* payload) const noexcept {
    const std::lock_guard lock(mutex_);
    return regions_.contains(payload);
}

std::size_t LargeAllocator::payloadCapacity(const void* payload) const noexcept {
    const std::lock_guard lock(mutex_);
    const auto it = regions_.find(payload);
    if (it == regions_.end()) {
        return 0;
    }
    return it->second.size() - kPayloadOffset;
}

} // namespace ma::detail
