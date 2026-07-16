#pragma once

#include <cstddef>
#include <memory>
#include <span>

namespace ma::detail {

class FreeList {
public:
    struct Node {
        Node* next;
    };

    FreeList() noexcept = default;

    void push(void* block) noexcept {
        auto* node = std::construct_at(static_cast<Node*>(block), Node{head_});
        head_ = node;
        ++size_;
    }

    [[nodiscard]] void* pop() noexcept {
        if (head_ == nullptr) {
            return nullptr;
        }
        Node* node = head_;
        head_ = node->next;
        --size_;
        return node;
    }

    [[nodiscard]] std::size_t popBatch(std::span<void*> out) noexcept {
        std::size_t count = 0;
        while (count < out.size()) {
            void* block = pop();
            if (block == nullptr) {
                break;
            }
            out[count] = block;
            ++count;
        }
        return count;
    }

    [[nodiscard]] bool empty() const noexcept { return head_ == nullptr; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }

private:
    Node* head_ = nullptr;
    std::size_t size_ = 0;
};

} // namespace ma::detail
