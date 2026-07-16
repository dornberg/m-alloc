#pragma once

#include "MAlloc/Error.hpp"

#include <functional>

namespace ma {

using ErrorHandler = std::function<void(Error, const void*)>;

struct AllocatorConfig {
    bool enableGuardBytes = false;
    bool enablePoisoning = false;
    bool enableLeakTracking = false;
    bool enableDoubleFreeDetection = true;
    bool enableThreadCache = true;
    bool abortOnError = false;
    ErrorHandler errorHandler{};

    [[nodiscard]] static AllocatorConfig release() noexcept {
        return AllocatorConfig{};
    }

    [[nodiscard]] static AllocatorConfig debug() noexcept {
        AllocatorConfig config{};
        config.enableGuardBytes = true;
        config.enablePoisoning = true;
        config.enableLeakTracking = true;
        config.enableDoubleFreeDetection = true;
        config.enableThreadCache = false;
        return config;
    }
};

} // namespace ma
