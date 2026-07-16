#pragma once

#include <cstddef>
#include <span>

namespace ma::detail {

class SystemMemory {
public:
    SystemMemory() = delete;

    [[nodiscard]] static std::span<std::byte> map(std::size_t size) noexcept;
    static void unmap(std::span<std::byte> region) noexcept;
    [[nodiscard]] static std::size_t pageSize() noexcept;
    [[nodiscard]] static std::size_t roundToPages(std::size_t size) noexcept;
};

class MappedRegion {
public:
    MappedRegion() noexcept = default;

    explicit MappedRegion(std::size_t size) noexcept : region_(SystemMemory::map(size)) {}

    ~MappedRegion() { reset(); }

    MappedRegion(MappedRegion&& other) noexcept : region_(other.release()) {}

    MappedRegion& operator=(MappedRegion&& other) noexcept {
        if (this != &other) {
            reset();
            region_ = other.release();
        }
        return *this;
    }

    MappedRegion(const MappedRegion&) = delete;
    MappedRegion& operator=(const MappedRegion&) = delete;

    [[nodiscard]] std::span<std::byte> get() const noexcept { return region_; }
    [[nodiscard]] std::byte* data() const noexcept { return region_.data(); }
    [[nodiscard]] std::size_t size() const noexcept { return region_.size(); }
    [[nodiscard]] bool valid() const noexcept { return !region_.empty(); }

    [[nodiscard]] std::span<std::byte> release() noexcept {
        auto result = region_;
        region_ = {};
        return result;
    }

    void reset() noexcept {
        if (!region_.empty()) {
            SystemMemory::unmap(region_);
            region_ = {};
        }
    }

private:
    std::span<std::byte> region_{};
};

} // namespace ma::detail
