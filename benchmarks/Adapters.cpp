#include "Adapters.hpp"

#include <cstdlib>
#include <memory_resource>

#if defined(MALLOC_HAVE_MIMALLOC)
    #include <mimalloc.h>
#endif

#if defined(MALLOC_HAVE_JEMALLOC)
    #include <jemalloc/jemalloc.h>
#endif

namespace ma::bench {

namespace {

class SystemMallocAdapter final : public AllocatorAdapter {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "std::malloc"; }

    [[nodiscard]] void* allocate(std::size_t size) override { return std::malloc(size); }

    void deallocate(void* ptr, std::size_t) override { std::free(ptr); }
};

class MAllocAdapter final : public AllocatorAdapter {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "m-alloc"; }

    [[nodiscard]] void* allocate(std::size_t size) override { return allocator_.allocate(size); }

    void deallocate(void* ptr, std::size_t) override { allocator_.deallocate(ptr); }

private:
    Allocator allocator_;
};

class PmrPoolAdapter final : public AllocatorAdapter {
public:
    [[nodiscard]] std::string_view name() const noexcept override {
        return "std::pmr::synchronized_pool_resource";
    }

    [[nodiscard]] void* allocate(std::size_t size) override { return pool_.allocate(size); }

    void deallocate(void* ptr, std::size_t size) override { pool_.deallocate(ptr, size); }

private:
    std::pmr::synchronized_pool_resource pool_;
};

#if defined(MALLOC_HAVE_MIMALLOC)
class MimallocAdapter final : public AllocatorAdapter {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "mimalloc"; }

    [[nodiscard]] void* allocate(std::size_t size) override { return mi_malloc(size); }

    void deallocate(void* ptr, std::size_t) override { mi_free(ptr); }
};
#endif

#if defined(MALLOC_HAVE_JEMALLOC)
class JemallocAdapter final : public AllocatorAdapter {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "jemalloc"; }

    [[nodiscard]] void* allocate(std::size_t size) override { return ::malloc(size); }

    void deallocate(void* ptr, std::size_t) override { ::free(ptr); }
};
#endif

} // namespace

std::vector<std::unique_ptr<AllocatorAdapter>> makeAdapters() {
    std::vector<std::unique_ptr<AllocatorAdapter>> adapters;
    adapters.push_back(std::make_unique<SystemMallocAdapter>());
    adapters.push_back(std::make_unique<PmrPoolAdapter>());
    adapters.push_back(std::make_unique<MAllocAdapter>());
#if defined(MALLOC_HAVE_MIMALLOC)
    adapters.push_back(std::make_unique<MimallocAdapter>());
#endif
#if defined(MALLOC_HAVE_JEMALLOC)
    adapters.push_back(std::make_unique<JemallocAdapter>());
#endif
    return adapters;
}

} // namespace ma::bench
