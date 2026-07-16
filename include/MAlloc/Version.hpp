#pragma once

#include <cstdint>
#include <string_view>

namespace ma {

inline constexpr std::uint32_t kVersionMajor = 0;
inline constexpr std::uint32_t kVersionMinor = 1;
inline constexpr std::uint32_t kVersionPatch = 0;

[[nodiscard]] constexpr std::string_view versionString() noexcept {
    return "0.1.0";
}

} // namespace ma
