#pragma once

#include "MAlloc/Config.hpp"

#include "LargeAllocator.hpp"
#include "PoolAllocator.hpp"
#include "SegmentHeap.hpp"
#include "StatisticsCollector.hpp"

#include <utility>

namespace ma::detail {

struct CentralState {
    explicit CentralState(AllocatorConfig configValue) : config(std::move(configValue)) {}

    AllocatorConfig config;
    StatisticsCollector stats;
    PoolAllocator pools;
    SegmentHeap segments;
    LargeAllocator large;
};

} // namespace ma::detail
