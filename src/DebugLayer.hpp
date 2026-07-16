#pragma once

#include "BlockHeader.hpp"

#include <cstddef>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace ma::detail {

class DebugLayer {
public:
    static constexpr std::size_t kFrontGuardSize = 32;
    static constexpr std::size_t kRearGuardSize = 32;
    static constexpr std::byte kGuardPattern{0xFB};
    static constexpr std::byte kAllocPoison{0xCD};
    static constexpr std::byte kFreePoison{0xDD};

    [[nodiscard]] static constexpr std::size_t guardedSize(std::size_t requested) noexcept {
        return requested + kFrontGuardSize + kRearGuardSize;
    }

    [[nodiscard]] static void* applyGuards(void* raw, std::size_t requested) noexcept {
        auto* base = static_cast<std::byte*>(raw);
        std::byte* user = base + kFrontGuardSize;
        std::memset(base, static_cast<int>(kGuardPattern), kFrontGuardSize - kHeaderSize);
        std::memset(user + requested, static_cast<int>(kGuardPattern), kRearGuardSize);
        auto* shim = headerOf(user);
        shim->payloadSize = requested;
        shim->tier = static_cast<std::uint16_t>(Tier::DebugShim);
        shim->state = kStateAllocated;
        shim->offset = static_cast<std::uint32_t>(kFrontGuardSize);
        return user;
    }

    [[nodiscard]] static bool verifyGuards(const void* user, std::size_t requested) noexcept {
        const auto* userBytes = static_cast<const std::byte*>(user);
        const std::byte* front = userBytes - kFrontGuardSize;
        for (std::size_t i = 0; i < kFrontGuardSize - kHeaderSize; ++i) {
            if (front[i] != kGuardPattern) {
                return false;
            }
        }
        const std::byte* rear = userBytes + requested;
        for (std::size_t i = 0; i < kRearGuardSize; ++i) {
            if (rear[i] != kGuardPattern) {
                return false;
            }
        }
        return true;
    }

    static void poisonOnAllocate(void* ptr, std::size_t size) noexcept {
        std::memset(ptr, static_cast<int>(kAllocPoison), size);
    }

    static void poisonOnFree(void* ptr, std::size_t size) noexcept {
        std::memset(ptr, static_cast<int>(kFreePoison), size);
    }

    void track(const void* user, std::size_t size) noexcept {
        const std::lock_guard lock(mutex_);
        try {
            live_.insert_or_assign(user, size);
            freed_.erase(user);
        } catch (...) {}
    }

    [[nodiscard]] bool untrack(const void* user) noexcept {
        const std::lock_guard lock(mutex_);
        if (live_.erase(user) == 0) {
            return false;
        }
        try {
            freed_.insert(user);
        } catch (...) {}
        return true;
    }

    [[nodiscard]] bool isTracked(const void* user) const noexcept {
        const std::lock_guard lock(mutex_);
        return live_.contains(user);
    }

    [[nodiscard]] bool wasFreed(const void* user) const noexcept {
        const std::lock_guard lock(mutex_);
        return freed_.contains(user);
    }

    [[nodiscard]] std::size_t liveCount() const noexcept {
        const std::lock_guard lock(mutex_);
        return live_.size();
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<const void*, std::size_t> live_;
    std::unordered_set<const void*> freed_;
};

} // namespace ma::detail
