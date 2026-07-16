#include "SegmentHeap.hpp"

#include <algorithm>
#include <bit>
#include <memory>

namespace ma::detail {

std::size_t SegmentHeap::binIndex(std::size_t size) noexcept {
    const auto width = static_cast<std::size_t>(std::bit_width(size));
    const std::size_t raw = width > 6 ? width - 6 : 0;
    return std::min(raw, kBinCount - 1);
}

SegmentHeap::MediumBlock* SegmentHeap::blockOf(void* payload) noexcept {
    return reinterpret_cast<MediumBlock*>(static_cast<std::byte*>(payload) - kBlockHeaderSize);
}

void* SegmentHeap::payloadOf(MediumBlock* block) noexcept {
    return reinterpret_cast<std::byte*>(block) + kBlockHeaderSize;
}

SegmentHeap::FreeNode* SegmentHeap::nodeOf(MediumBlock* block) noexcept {
    return reinterpret_cast<FreeNode*>(reinterpret_cast<std::byte*>(block) + kNodeOffset);
}

SegmentHeap::MediumBlock* SegmentHeap::blockOfNode(FreeNode* node) noexcept {
    return reinterpret_cast<MediumBlock*>(reinterpret_cast<std::byte*>(node) - kNodeOffset);
}

SegmentHeap::MediumBlock* SegmentHeap::nextBlock(MediumBlock* block) noexcept {
    return reinterpret_cast<MediumBlock*>(reinterpret_cast<std::byte*>(block) + block->size());
}

SegmentHeap::MediumBlock* SegmentHeap::prevBlock(MediumBlock* block) noexcept {
    return reinterpret_cast<MediumBlock*>(reinterpret_cast<std::byte*>(block) - block->prevSize);
}

void* SegmentHeap::allocate(std::size_t bytes) noexcept {
    std::size_t total = alignUp(bytes, kDefaultAlignment) + kBlockHeaderSize;
    total = std::max(total, kMinBlockSize);
    if (total > kSegmentSize) {
        return nullptr;
    }
    const std::lock_guard lock(mutex_);
    MediumBlock* block = findFit(total);
    if (block == nullptr) {
        if (!addSegment()) {
            return nullptr;
        }
        block = findFit(total);
        if (block == nullptr) {
            return nullptr;
        }
    }
    removeFree(block);
    splitBlock(block, total);
    return payloadOf(block);
}

void SegmentHeap::deallocate(void* payload) noexcept {
    const std::lock_guard lock(mutex_);
    MediumBlock* block = blockOf(payload);
    if (!block->isLast()) {
        MediumBlock* next = nextBlock(block);
        if (next->isFree()) {
            removeFree(next);
            block->setSize(block->size() + next->size());
            block->setFlag(MediumBlock::kLastFlag, next->isLast());
        }
    }
    if (!block->isFirst()) {
        MediumBlock* prev = prevBlock(block);
        if (prev->isFree()) {
            removeFree(prev);
            prev->setSize(prev->size() + block->size());
            prev->setFlag(MediumBlock::kLastFlag, block->isLast());
            block = prev;
        }
    }
    if (!block->isLast()) {
        nextBlock(block)->prevSize = block->size();
    }
    if (block->isFirst() && block->isLast()) {
        releaseSegment(block);
    } else {
        insertFree(block);
    }
}

std::size_t SegmentHeap::payloadCapacity(const void* payload) const noexcept {
    const std::lock_guard lock(mutex_);
    const auto* block = reinterpret_cast<const MediumBlock*>(
        static_cast<const std::byte*>(payload) - kBlockHeaderSize);
    return block->size() - kBlockHeaderSize;
}

double SegmentHeap::externalFragmentation() const noexcept {
    const std::lock_guard lock(mutex_);
    std::uint64_t totalFree = 0;
    std::uint64_t largestFree = 0;
    for (FreeNode* node : bins_) {
        while (node != nullptr) {
            const auto* block = reinterpret_cast<const MediumBlock*>(
                reinterpret_cast<const std::byte*>(node) - kNodeOffset);
            const std::uint64_t size = block->size();
            totalFree += size;
            largestFree = std::max(largestFree, size);
            node = node->next;
        }
    }
    if (totalFree == 0) {
        return 0.0;
    }
    return 1.0 - static_cast<double>(largestFree) / static_cast<double>(totalFree);
}

void SegmentHeap::insertFree(MediumBlock* block) noexcept {
    const std::size_t index = binIndex(block->size());
    auto* node = std::construct_at(nodeOf(block), FreeNode{nullptr, bins_[index]});
    if (node->next != nullptr) {
        node->next->prev = node;
    }
    bins_[index] = node;
    block->setFlag(MediumBlock::kFreeFlag, true);
}

void SegmentHeap::removeFree(MediumBlock* block) noexcept {
    FreeNode* node = nodeOf(block);
    if (node->prev != nullptr) {
        node->prev->next = node->next;
    } else {
        bins_[binIndex(block->size())] = node->next;
    }
    if (node->next != nullptr) {
        node->next->prev = node->prev;
    }
    block->setFlag(MediumBlock::kFreeFlag, false);
}

SegmentHeap::MediumBlock* SegmentHeap::findFit(std::size_t totalSize) noexcept {
    for (std::size_t index = binIndex(totalSize); index < kBinCount; ++index) {
        for (FreeNode* node = bins_[index]; node != nullptr; node = node->next) {
            MediumBlock* block = blockOfNode(node);
            if (block->size() >= totalSize) {
                return block;
            }
        }
    }
    return nullptr;
}

bool SegmentHeap::addSegment() noexcept {
    MappedRegion segment(kSegmentSize);
    if (!segment.valid()) {
        return false;
    }
    auto* block = reinterpret_cast<MediumBlock*>(segment.data());
    block->sizeAndFlags = 0;
    block->setSize(segment.size());
    block->setFlag(MediumBlock::kFirstFlag, true);
    block->setFlag(MediumBlock::kLastFlag, true);
    block->prevSize = 0;
    insertFree(block);
    segments_.push_back(std::move(segment));
    return true;
}

void SegmentHeap::splitBlock(MediumBlock* block, std::size_t totalSize) noexcept {
    const std::size_t remainder = block->size() - totalSize;
    if (remainder < kMinBlockSize) {
        return;
    }
    auto* rest = reinterpret_cast<MediumBlock*>(reinterpret_cast<std::byte*>(block) + totalSize);
    rest->sizeAndFlags = 0;
    rest->setSize(remainder);
    rest->setFlag(MediumBlock::kLastFlag, block->isLast());
    rest->prevSize = totalSize;
    block->setSize(totalSize);
    block->setFlag(MediumBlock::kLastFlag, false);
    if (!rest->isLast()) {
        nextBlock(rest)->prevSize = remainder;
    }
    insertFree(rest);
}

void SegmentHeap::releaseSegment(MediumBlock* block) noexcept {
    if (segments_.size() <= 1) {
        insertFree(block);
        return;
    }
    auto* base = reinterpret_cast<std::byte*>(block);
    for (auto it = segments_.begin(); it != segments_.end(); ++it) {
        if (it->data() == base) {
            segments_.erase(it);
            return;
        }
    }
    insertFree(block);
}

} // namespace ma::detail
