#pragma once

#include "MAlloc/Allocator.hpp"

#include <cstddef>
#include <memory_resource>
#include <new>

namespace ma {

class MemoryResource final : public std::pmr::memory_resource {
public:
    explicit MemoryResource(Allocator& allocator) noexcept : allocator_(&allocator) {}

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        void* ptr = alignment > 16 ? allocator_->alignedAllocate(alignment, bytes)
                                   : allocator_->allocate(bytes);
        if (ptr == nullptr) {
            throw std::bad_alloc{};
        }
        return ptr;
    }

    void do_deallocate(void* ptr, std::size_t, std::size_t) override {
        allocator_->deallocate(ptr);
    }

    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

    Allocator* allocator_;
};

} // namespace ma
