#include "SystemMemory.hpp"

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
#endif

namespace ma::detail {

std::span<std::byte> SystemMemory::map(std::size_t size) noexcept {
    if (size == 0) {
        return {};
    }
    const std::size_t rounded = roundToPages(size);
#if defined(_WIN32)
    void* ptr = ::VirtualAlloc(nullptr, rounded, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (ptr == nullptr) {
        return {};
    }
#else
    void* ptr = ::mmap(nullptr, rounded, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        return {};
    }
#endif
    return {static_cast<std::byte*>(ptr), rounded};
}

void SystemMemory::unmap(std::span<std::byte> region) noexcept {
    if (region.empty()) {
        return;
    }
#if defined(_WIN32)
    ::VirtualFree(region.data(), 0, MEM_RELEASE);
#else
    ::munmap(region.data(), region.size());
#endif
}

std::size_t SystemMemory::pageSize() noexcept {
#if defined(_WIN32)
    static const std::size_t cached = [] {
        SYSTEM_INFO info{};
        ::GetSystemInfo(&info);
        return static_cast<std::size_t>(info.dwPageSize);
    }();
#else
    static const std::size_t cached = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
#endif
    return cached;
}

std::size_t SystemMemory::roundToPages(std::size_t size) noexcept {
    const std::size_t page = pageSize();
    return (size + page - 1) / page * page;
}

} // namespace ma::detail
