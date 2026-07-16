#pragma once

#include <cstdint>
#include <string_view>

namespace ma {

enum class Error : std::uint8_t {
    OutOfMemory,
    SizeOverflow,
    InvalidAlignment,
    InvalidPointer,
    DoubleFree,
    HeapCorruption
};

[[nodiscard]] constexpr std::string_view toString(Error error) noexcept {
    switch (error) {
    case Error::OutOfMemory:
        return "out of memory";
    case Error::SizeOverflow:
        return "size overflow";
    case Error::InvalidAlignment:
        return "invalid alignment";
    case Error::InvalidPointer:
        return "invalid pointer";
    case Error::DoubleFree:
        return "double free";
    case Error::HeapCorruption:
        return "heap corruption";
    }
    return "unknown error";
}

} // namespace ma
