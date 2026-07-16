# Allocator API

All public types live in namespace `ma` and are exported through the umbrella header `MAlloc/MAlloc.hpp`.

## ma::Allocator

The facade. Each instance owns an independent heap; instances never share state. The class is movable, not copyable, and every operation is `noexcept`.

### Construction

```cpp
ma::Allocator allocator;                                  // release defaults
ma::Allocator debugAllocator(ma::AllocatorConfig::debug());
```

### Core API

| Method | Behavior |
|---|---|
| `void* allocate(std::size_t size)` | Returns a 16-byte aligned block, or `nullptr` on failure. Size 0 is treated as 1. |
| `void deallocate(void* ptr)` | Frees a block. `nullptr` is a no-op. Errors are reported through the error handler. |
| `void* reallocate(void* ptr, std::size_t newSize)` | Grows or shrinks. `nullptr` behaves like `allocate`, size 0 frees and returns `nullptr`. Contents preserved up to `min(old, new)`. |
| `void* alignedAllocate(std::size_t alignment, std::size_t size)` | Any power-of-two alignment. Alignments <= 16 use the regular path. |

### Expected-based API

Same semantics, explicit errors:

```cpp
std::expected<void*, ma::Error> tryAllocate(std::size_t size);
std::expected<void, ma::Error>  tryDeallocate(void* ptr);
std::expected<void*, ma::Error> tryAlignedAllocate(std::size_t alignment, std::size_t size);
```

### Introspection

| Method | Behavior |
|---|---|
| `std::size_t allocationSize(const void* ptr)` | Requested size of a live allocation, 0 for unknown pointers. |
| `ma::StatisticsSnapshot statistics()` | Consistent snapshot of runtime counters. |
| `const ma::AllocatorConfig& config()` | Active configuration. |
| `std::size_t liveAllocationCount()` | Live allocations when leak tracking is enabled, otherwise 0. |

## ma::Error

```cpp
enum class Error : std::uint8_t {
    OutOfMemory, SizeOverflow, InvalidAlignment,
    InvalidPointer, DoubleFree, HeapCorruption
};
constexpr std::string_view toString(Error);
```

## ma::AllocatorConfig

| Field | Default | Effect |
|---|---|---|
| `enableGuardBytes` | `false` | Guard zones around each allocation, verified on free |
| `enablePoisoning` | `false` | Fill memory with `0xCD` on allocate and `0xDD` on free |
| `enableLeakTracking` | `false` | Track live pointers, enables safe invalid-pointer detection |
| `enableDoubleFreeDetection` | `true` | Header state validation on free |
| `enableThreadCache` | `true` | Thread-local caches for small allocations |
| `abortOnError` | `false` | `std::abort()` after reporting an error |
| `errorHandler` | empty | `std::function<void(Error, const void*)>`; default writes to stderr |

Factory presets: `AllocatorConfig::release()` (all debug features off, thread cache on) and `AllocatorConfig::debug()` (guards, poisoning, leak tracking on, thread cache off).

## ma::StlAllocator\<T\>

Standard-conforming allocator bound to an `ma::Allocator` instance. Throws `std::bad_alloc` on failure as containers require. Two instances compare equal when they reference the same allocator.

## ma::MemoryResource

`std::pmr::memory_resource` adapter over an `ma::Allocator`. Honors the alignment argument of `do_allocate`.

## Thread safety

All `ma::Allocator` methods are safe to call concurrently from any number of threads. A block may be allocated in one thread and freed in another. The allocator instance must outlive every pointer it produced.
