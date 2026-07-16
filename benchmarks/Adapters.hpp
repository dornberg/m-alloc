#pragma once

#include "MAlloc/MAlloc.hpp"

#include <cstddef>
#include <memory>
#include <string_view>
#include <vector>

namespace ma::bench {

class AllocatorAdapter {
public:
    virtual ~AllocatorAdapter() = default;

    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    [[nodiscard]] virtual void* allocate(std::size_t size) = 0;
    virtual void deallocate(void* ptr, std::size_t size) = 0;
    [[nodiscard]] virtual bool threadSafe() const noexcept { return true; }
};

[[nodiscard]] std::vector<std::unique_ptr<AllocatorAdapter>> makeAdapters();

} // namespace ma::bench
