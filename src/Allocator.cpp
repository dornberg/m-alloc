#include "MAlloc/Allocator.hpp"

#include "AllocatorImpl.hpp"

namespace ma {

namespace {

const AllocatorConfig kFallbackConfig{};

}

Allocator::Allocator() : impl_(std::make_unique<detail::AllocatorImpl>(AllocatorConfig{})) {}

Allocator::Allocator(AllocatorConfig config)
    : impl_(std::make_unique<detail::AllocatorImpl>(std::move(config))) {}

Allocator::~Allocator() = default;

Allocator::Allocator(Allocator&& other) noexcept = default;

Allocator& Allocator::operator=(Allocator&& other) noexcept = default;

void* Allocator::allocate(std::size_t size) noexcept {
    if (!impl_) {
        return nullptr;
    }
    return impl_->allocate(size).value_or(nullptr);
}

void Allocator::deallocate(void* ptr) noexcept {
    if (impl_) {
        static_cast<void>(impl_->deallocate(ptr));
    }
}

void* Allocator::reallocate(void* ptr, std::size_t newSize) noexcept {
    if (!impl_) {
        return nullptr;
    }
    return impl_->reallocate(ptr, newSize).value_or(nullptr);
}

void* Allocator::alignedAllocate(std::size_t alignment, std::size_t size) noexcept {
    if (!impl_) {
        return nullptr;
    }
    return impl_->alignedAllocate(alignment, size).value_or(nullptr);
}

std::expected<void*, Error> Allocator::tryAllocate(std::size_t size) noexcept {
    if (!impl_) {
        return std::unexpected(Error::OutOfMemory);
    }
    return impl_->allocate(size);
}

std::expected<void, Error> Allocator::tryDeallocate(void* ptr) noexcept {
    if (!impl_) {
        return std::unexpected(Error::InvalidPointer);
    }
    return impl_->deallocate(ptr);
}

std::expected<void*, Error> Allocator::tryAlignedAllocate(std::size_t alignment,
                                                          std::size_t size) noexcept {
    if (!impl_) {
        return std::unexpected(Error::OutOfMemory);
    }
    return impl_->alignedAllocate(alignment, size);
}

std::size_t Allocator::allocationSize(const void* ptr) const noexcept {
    if (!impl_) {
        return 0;
    }
    return impl_->allocationSize(ptr);
}

StatisticsSnapshot Allocator::statistics() const noexcept {
    if (!impl_) {
        return {};
    }
    return impl_->statistics();
}

const AllocatorConfig& Allocator::config() const noexcept {
    if (!impl_) {
        return kFallbackConfig;
    }
    return impl_->config();
}

std::size_t Allocator::liveAllocationCount() const noexcept {
    if (!impl_) {
        return 0;
    }
    return impl_->liveAllocationCount();
}

} // namespace ma
