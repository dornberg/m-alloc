#include "AllocatorImpl.hpp"

#include "SizeClassTable.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>

namespace ma::detail {

namespace {

thread_local std::unordered_map<const CentralState*, std::unique_ptr<ThreadCache>> tlsCaches;

}

AllocatorImpl::AllocatorImpl(AllocatorConfig config)
    : central_(std::make_shared<CentralState>(std::move(config))) {}

AllocatorImpl::~AllocatorImpl() {
    if (central_->config.enableLeakTracking) {
        const std::size_t leaks = debug_.liveCount();
        if (leaks > 0) {
            std::fprintf(stderr, "m-alloc: %zu allocation(s) leaked\n", leaks);
        }
    }
}

std::expected<void*, Error> AllocatorImpl::allocate(std::size_t size) noexcept {
    if (size == 0) {
        size = 1;
    }
    if (size > kMaxRequest) {
        report(Error::SizeOverflow, nullptr);
        return std::unexpected(Error::SizeOverflow);
    }
    const AllocatorConfig& cfg = central_->config;
    void* user = nullptr;
    std::size_t reserved = 0;
    if (cfg.enableGuardBytes) {
        auto raw = rawAllocate(DebugLayer::guardedSize(size));
        if (!raw) {
            report(raw.error(), nullptr);
            return std::unexpected(raw.error());
        }
        user = DebugLayer::applyGuards(raw->user, size);
        reserved = raw->reserved;
    } else {
        auto raw = rawAllocate(size);
        if (!raw) {
            report(raw.error(), nullptr);
            return std::unexpected(raw.error());
        }
        user = raw->user;
        reserved = raw->reserved;
    }
    if (cfg.enablePoisoning) {
        DebugLayer::poisonOnAllocate(user, size);
    }
    central_->stats.recordAllocation(size, reserved);
    if (cfg.enableLeakTracking) {
        debug_.track(user, size);
    }
    return user;
}

std::expected<void, Error> AllocatorImpl::deallocate(void* ptr) noexcept {
    if (ptr == nullptr) {
        return {};
    }
    const AllocatorConfig& cfg = central_->config;
    if (cfg.enableLeakTracking && !debug_.isTracked(ptr)) {
        const Error error = debug_.wasFreed(ptr) ? Error::DoubleFree : Error::InvalidPointer;
        report(error, ptr);
        return std::unexpected(error);
    }
    BlockHeader* header = headerOf(ptr);
    if (!header->hasValidTier() ||
        (header->state != kStateAllocated && header->state != kStateFreed)) {
        report(Error::InvalidPointer, ptr);
        return std::unexpected(Error::InvalidPointer);
    }
    if (header->state == kStateFreed) {
        report(Error::DoubleFree, ptr);
        return std::unexpected(Error::DoubleFree);
    }
    const std::size_t requested = header->payloadSize;
    std::expected<std::size_t, Error> reserved;
    switch (header->tierValue()) {
    case Tier::DebugShim: {
        if (cfg.enableGuardBytes && !DebugLayer::verifyGuards(ptr, requested)) {
            report(Error::HeapCorruption, ptr);
        }
        if (cfg.enablePoisoning) {
            DebugLayer::poisonOnFree(ptr, requested);
        }
        header->state = kStateFreed;
        reserved = rawDeallocate(static_cast<std::byte*>(ptr) - header->offset);
        break;
    }
    case Tier::AlignedShim: {
        if (cfg.enablePoisoning) {
            DebugLayer::poisonOnFree(ptr, requested);
        }
        header->state = kStateFreed;
        reserved = rawDeallocate(static_cast<std::byte*>(ptr) - header->offset);
        break;
    }
    default: {
        if (cfg.enablePoisoning) {
            DebugLayer::poisonOnFree(ptr, requested);
        }
        reserved = rawDeallocate(ptr);
        break;
    }
    }
    if (!reserved) {
        report(reserved.error(), ptr);
        return std::unexpected(reserved.error());
    }
    central_->stats.recordFree(requested, *reserved);
    if (cfg.enableLeakTracking) {
        static_cast<void>(debug_.untrack(ptr));
    }
    return {};
}

std::expected<void*, Error> AllocatorImpl::reallocate(void* ptr, std::size_t newSize) noexcept {
    if (ptr == nullptr) {
        return allocate(newSize);
    }
    if (newSize == 0) {
        auto freed = deallocate(ptr);
        if (!freed) {
            return std::unexpected(freed.error());
        }
        return nullptr;
    }
    const std::size_t oldSize = allocationSize(ptr);
    if (oldSize == 0) {
        report(Error::InvalidPointer, ptr);
        return std::unexpected(Error::InvalidPointer);
    }
    auto fresh = allocate(newSize);
    if (!fresh) {
        return std::unexpected(fresh.error());
    }
    std::memcpy(*fresh, ptr, std::min(oldSize, newSize));
    auto freed = deallocate(ptr);
    if (!freed) {
        return std::unexpected(freed.error());
    }
    return *fresh;
}

std::expected<void*, Error> AllocatorImpl::alignedAllocate(std::size_t alignment,
                                                           std::size_t size) noexcept {
    if (!isPowerOfTwo(alignment) || alignment > kMaxAlignment) {
        report(Error::InvalidAlignment, nullptr);
        return std::unexpected(Error::InvalidAlignment);
    }
    if (alignment <= kDefaultAlignment) {
        return allocate(size);
    }
    if (size == 0) {
        size = 1;
    }
    if (size > kMaxRequest) {
        report(Error::SizeOverflow, nullptr);
        return std::unexpected(Error::SizeOverflow);
    }
    auto raw = rawAllocate(size + alignment);
    if (!raw) {
        report(raw.error(), nullptr);
        return std::unexpected(raw.error());
    }
    auto address = reinterpret_cast<std::uintptr_t>(raw->user);
    std::uintptr_t alignedAddress = alignUp(address, alignment);
    if (alignedAddress == address) {
        alignedAddress += alignment;
    }
    auto* user = reinterpret_cast<std::byte*>(alignedAddress);
    BlockHeader* shim = headerOf(user);
    shim->payloadSize = size;
    shim->tier = static_cast<std::uint16_t>(Tier::AlignedShim);
    shim->state = kStateAllocated;
    shim->offset = static_cast<std::uint32_t>(alignedAddress - address);
    if (central_->config.enablePoisoning) {
        DebugLayer::poisonOnAllocate(user, size);
    }
    central_->stats.recordAllocation(size, raw->reserved);
    if (central_->config.enableLeakTracking) {
        debug_.track(user, size);
    }
    return user;
}

std::size_t AllocatorImpl::allocationSize(const void* ptr) const noexcept {
    if (ptr == nullptr) {
        return 0;
    }
    if (central_->config.enableLeakTracking && !debug_.isTracked(ptr)) {
        return 0;
    }
    const BlockHeader* header = headerOf(ptr);
    if (!header->hasValidTier() || header->state != kStateAllocated) {
        return 0;
    }
    return header->payloadSize;
}

StatisticsSnapshot AllocatorImpl::statistics() const noexcept {
    return central_->stats.snapshot(central_->segments.externalFragmentation());
}

std::expected<AllocatorImpl::RawAllocation, Error>
AllocatorImpl::rawAllocate(std::size_t size) noexcept {
    if (size <= kSmallMax) {
        const std::size_t classIndex = SizeClassTable::classIndex(size);
        void* block = nullptr;
        if (ThreadCache* cache = threadCache(); cache != nullptr) {
            block = cache->allocateBlock(classIndex, central_->pools);
        } else {
            block = central_->pools.allocateBlock(classIndex);
        }
        if (block == nullptr) {
            return std::unexpected(Error::OutOfMemory);
        }
        void* user = userPointerOf(block);
        auto* header = headerOf(user);
        header->payloadSize = size;
        header->tier = static_cast<std::uint16_t>(Tier::Small);
        header->state = kStateAllocated;
        header->offset = 0;
        return RawAllocation{user, SizeClassTable::classSize(classIndex)};
    }
    if (size <= kMediumMax) {
        void* payload = central_->segments.allocate(size + kHeaderSize);
        if (payload == nullptr) {
            return std::unexpected(Error::OutOfMemory);
        }
        void* user = userPointerOf(payload);
        auto* header = headerOf(user);
        header->payloadSize = size;
        header->tier = static_cast<std::uint16_t>(Tier::Medium);
        header->state = kStateAllocated;
        header->offset = 0;
        return RawAllocation{user, central_->segments.payloadCapacity(payload) - kHeaderSize};
    }
    void* payload = central_->large.allocate(size + kHeaderSize);
    if (payload == nullptr) {
        return std::unexpected(Error::OutOfMemory);
    }
    void* user = userPointerOf(payload);
    auto* header = headerOf(user);
    header->payloadSize = size;
    header->tier = static_cast<std::uint16_t>(Tier::Large);
    header->state = kStateAllocated;
    header->offset = 0;
    return RawAllocation{user, central_->large.payloadCapacity(payload) - kHeaderSize};
}

std::expected<std::size_t, Error> AllocatorImpl::rawDeallocate(void* raw) noexcept {
    BlockHeader* header = headerOf(raw);
    switch (header->tierValue()) {
    case Tier::Small: {
        const std::size_t classIndex = SizeClassTable::classIndex(header->payloadSize);
        header->state = kStateFreed;
        void* block = header;
        if (ThreadCache* cache = threadCache(); cache != nullptr) {
            cache->freeBlock(classIndex, block, central_->pools);
        } else {
            central_->pools.freeBlock(classIndex, block);
        }
        return SizeClassTable::classSize(classIndex);
    }
    case Tier::Medium: {
        header->state = kStateFreed;
        void* payload = header;
        const std::size_t reserved = central_->segments.payloadCapacity(payload) - kHeaderSize;
        central_->segments.deallocate(payload);
        return reserved;
    }
    case Tier::Large: {
        void* payload = header;
        const std::size_t reserved = central_->large.payloadCapacity(payload) - kHeaderSize;
        if (!central_->large.deallocate(payload)) {
            return std::unexpected(Error::InvalidPointer);
        }
        return reserved;
    }
    default:
        return std::unexpected(Error::InvalidPointer);
    }
}

ThreadCache* AllocatorImpl::threadCache() noexcept {
    if (!central_->config.enableThreadCache) {
        return nullptr;
    }
    try {
        auto& slot = tlsCaches[central_.get()];
        if (!slot || slot->expired()) {
            slot = std::make_unique<ThreadCache>(central_);
        }
        return slot.get();
    } catch (...) {
        return nullptr;
    }
}

void AllocatorImpl::report(Error error, const void* ptr) const noexcept {
    const AllocatorConfig& cfg = central_->config;
    if (cfg.errorHandler) {
        try {
            cfg.errorHandler(error, ptr);
        } catch (...) {}
    } else {
        std::fprintf(stderr,
                     "m-alloc: %.*s (%p)\n",
                     static_cast<int>(toString(error).size()),
                     toString(error).data(),
                     ptr);
    }
    if (cfg.abortOnError) {
        std::abort();
    }
}

} // namespace ma::detail
