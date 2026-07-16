#pragma once

#include "SystemMemory.hpp"

#include <cstddef>
#include <mutex>
#include <unordered_map>

namespace ma::detail {

class LargeAllocator {
public:
    LargeAllocator() = default;
    ~LargeAllocator() = default;

    LargeAllocator(const LargeAllocator&) = delete;
    LargeAllocator& operator=(const LargeAllocator&) = delete;
    LargeAllocator(LargeAllocator&&) = delete;
    LargeAllocator& operator=(LargeAllocator&&) = delete;

    [[nodiscard]] void* allocate(std::size_t bytes) noexcept;
    [[nodiscard]] bool deallocate(void* payload) noexcept;
    [[nodiscard]] bool contains(const void* payload) const noexcept;
    [[nodiscard]] std::size_t payloadCapacity(const void* payload) const noexcept;

private:
    static constexpr std::size_t kPayloadOffset = 16;

    mutable std::mutex mutex_;
    std::unordered_map<const void*, MappedRegion> regions_;
};

} // namespace ma::detail
