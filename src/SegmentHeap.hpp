#pragma once

#include "BlockHeader.hpp"
#include "SystemMemory.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

namespace ma::detail {

class SegmentHeap {
public:
    SegmentHeap() = default;
    ~SegmentHeap() = default;

    SegmentHeap(const SegmentHeap&) = delete;
    SegmentHeap& operator=(const SegmentHeap&) = delete;
    SegmentHeap(SegmentHeap&&) = delete;
    SegmentHeap& operator=(SegmentHeap&&) = delete;

    [[nodiscard]] void* allocate(std::size_t bytes) noexcept;
    void deallocate(void* payload) noexcept;
    [[nodiscard]] std::size_t payloadCapacity(const void* payload) const noexcept;
    [[nodiscard]] double externalFragmentation() const noexcept;

private:
    struct MediumBlock {
        std::uint64_t sizeAndFlags;
        std::uint64_t prevSize;

        static constexpr std::uint64_t kFreeFlag = 1;
        static constexpr std::uint64_t kFirstFlag = 2;
        static constexpr std::uint64_t kLastFlag = 4;
        static constexpr std::uint64_t kFlagMask = 15;

        [[nodiscard]] std::size_t size() const noexcept {
            return static_cast<std::size_t>(sizeAndFlags & ~kFlagMask);
        }

        void setSize(std::size_t size) noexcept {
            sizeAndFlags = (sizeAndFlags & kFlagMask) | static_cast<std::uint64_t>(size);
        }

        [[nodiscard]] bool isFree() const noexcept { return (sizeAndFlags & kFreeFlag) != 0; }
        [[nodiscard]] bool isFirst() const noexcept { return (sizeAndFlags & kFirstFlag) != 0; }
        [[nodiscard]] bool isLast() const noexcept { return (sizeAndFlags & kLastFlag) != 0; }

        void setFlag(std::uint64_t flag, bool value) noexcept {
            if (value) {
                sizeAndFlags |= flag;
            } else {
                sizeAndFlags &= ~flag;
            }
        }
    };

    struct FreeNode {
        FreeNode* prev;
        FreeNode* next;
    };

    static constexpr std::size_t kSegmentSize = 4 * 1024 * 1024;
    static constexpr std::size_t kBlockHeaderSize = 16;
    static constexpr std::size_t kNodeOffset = 32;
    static constexpr std::size_t kMinBlockSize = 48;
    static constexpr std::size_t kBinCount = 18;

    static_assert(sizeof(MediumBlock) == kBlockHeaderSize);

    [[nodiscard]] static std::size_t binIndex(std::size_t size) noexcept;
    [[nodiscard]] static MediumBlock* blockOf(void* payload) noexcept;
    [[nodiscard]] static void* payloadOf(MediumBlock* block) noexcept;
    [[nodiscard]] static FreeNode* nodeOf(MediumBlock* block) noexcept;
    [[nodiscard]] static MediumBlock* blockOfNode(FreeNode* node) noexcept;
    [[nodiscard]] static MediumBlock* nextBlock(MediumBlock* block) noexcept;
    [[nodiscard]] static MediumBlock* prevBlock(MediumBlock* block) noexcept;

    void insertFree(MediumBlock* block) noexcept;
    void removeFree(MediumBlock* block) noexcept;
    [[nodiscard]] MediumBlock* findFit(std::size_t totalSize) noexcept;
    [[nodiscard]] bool addSegment() noexcept;
    void splitBlock(MediumBlock* block, std::size_t totalSize) noexcept;
    void releaseSegment(MediumBlock* block) noexcept;

    mutable std::mutex mutex_;
    std::array<FreeNode*, kBinCount> bins_{};
    std::vector<MappedRegion> segments_;
};

} // namespace ma::detail
