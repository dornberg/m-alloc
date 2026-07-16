#pragma once

#include "BlockHeader.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace ma::detail {

class SizeClassTable {
public:
    SizeClassTable() = delete;

    static constexpr std::size_t kClassCount = 30;

    static constexpr std::array<std::size_t, kClassCount> kClassSizes = {
        8,   16,  24,  32,  48,  64,  80,   96,   112,  128,  160,  192,  224,  256,  320,
        384, 448, 512, 640, 768, 896, 1024, 1280, 1536, 1792, 2048, 2560, 3072, 3584, 4096};

    [[nodiscard]] static constexpr std::size_t classIndex(std::size_t size) noexcept {
        return kLookup[(size + kGranularity - 1) / kGranularity];
    }

    [[nodiscard]] static constexpr std::size_t classSize(std::size_t index) noexcept {
        return kClassSizes[index];
    }

private:
    static constexpr std::size_t kGranularity = 8;
    static constexpr std::size_t kLookupSize = kSmallMax / kGranularity + 1;

    static constexpr std::array<std::uint8_t, kLookupSize> kLookup = [] {
        std::array<std::uint8_t, kLookupSize> table{};
        std::size_t classIdx = 0;
        for (std::size_t slot = 0; slot < kLookupSize; ++slot) {
            const std::size_t size = slot * kGranularity;
            while (kClassSizes[classIdx] < size) {
                ++classIdx;
            }
            table[slot] = static_cast<std::uint8_t>(classIdx);
        }
        return table;
    }();
};

static_assert(SizeClassTable::classIndex(1) == 0);
static_assert(SizeClassTable::classIndex(8) == 0);
static_assert(SizeClassTable::classIndex(9) == 1);
static_assert(SizeClassTable::classIndex(4096) == SizeClassTable::kClassCount - 1);

} // namespace ma::detail
