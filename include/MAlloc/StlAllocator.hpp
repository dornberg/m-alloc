#pragma once

#include "MAlloc/Allocator.hpp"

#include <cstddef>
#include <limits>
#include <new>
#include <type_traits>

namespace ma {

template <typename T>
class StlAllocator {
public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;
    using is_always_equal = std::false_type;

    explicit StlAllocator(Allocator& allocator) noexcept : allocator_(&allocator) {}

    template <typename U>
    StlAllocator(const StlAllocator<U>& other) noexcept : allocator_(other.underlying()) {}

    [[nodiscard]] T* allocate(size_type count) {
        if (count > std::numeric_limits<size_type>::max() / sizeof(T)) {
            throw std::bad_alloc{};
        }
        const size_type bytes = count * sizeof(T);
        void* ptr = nullptr;
        if constexpr (alignof(T) > 16) {
            ptr = allocator_->alignedAllocate(alignof(T), bytes);
        } else {
            ptr = allocator_->allocate(bytes);
        }
        if (ptr == nullptr) {
            throw std::bad_alloc{};
        }
        return static_cast<T*>(ptr);
    }

    void deallocate(T* ptr, size_type) noexcept { allocator_->deallocate(ptr); }

    [[nodiscard]] Allocator* underlying() const noexcept { return allocator_; }

    template <typename U>
    [[nodiscard]] bool operator==(const StlAllocator<U>& other) const noexcept {
        return allocator_ == other.underlying();
    }

private:
    Allocator* allocator_;
};

} // namespace ma
