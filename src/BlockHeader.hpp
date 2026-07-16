#pragma once

#include <cstddef>
#include <cstdint>

namespace ma::detail {

inline constexpr std::size_t kHeaderSize = 16;
inline constexpr std::size_t kDefaultAlignment = 16;
inline constexpr std::size_t kSmallMax = 4096;
inline constexpr std::size_t kMediumMax = 256 * 1024;

inline constexpr std::uint16_t kStateAllocated = 0xA11C;
inline constexpr std::uint16_t kStateFreed = 0xF4EE;

enum class Tier : std::uint16_t {
    Small = 0x51,
    Medium = 0x52,
    Large = 0x53,
    AlignedShim = 0x54,
    DebugShim = 0x55
};

struct BlockHeader {
    std::uint64_t payloadSize;
    std::uint16_t tier;
    std::uint16_t state;
    std::uint32_t offset;

    [[nodiscard]] bool hasValidTier() const noexcept {
        return tier >= static_cast<std::uint16_t>(Tier::Small) &&
               tier <= static_cast<std::uint16_t>(Tier::DebugShim);
    }

    [[nodiscard]] Tier tierValue() const noexcept { return static_cast<Tier>(tier); }
};

static_assert(sizeof(BlockHeader) == kHeaderSize);

[[nodiscard]] inline BlockHeader* headerOf(void* userPtr) noexcept {
    return reinterpret_cast<BlockHeader*>(static_cast<std::byte*>(userPtr) - kHeaderSize);
}

[[nodiscard]] inline const BlockHeader* headerOf(const void* userPtr) noexcept {
    return reinterpret_cast<const BlockHeader*>(static_cast<const std::byte*>(userPtr) -
                                                kHeaderSize);
}

[[nodiscard]] inline void* userPointerOf(void* blockStart) noexcept {
    return static_cast<std::byte*>(blockStart) + kHeaderSize;
}

[[nodiscard]] constexpr std::size_t alignUp(std::size_t value, std::size_t alignment) noexcept {
    return (value + alignment - 1) & ~(alignment - 1);
}

[[nodiscard]] constexpr bool isPowerOfTwo(std::size_t value) noexcept {
    return value != 0 && (value & (value - 1)) == 0;
}

} // namespace ma::detail
