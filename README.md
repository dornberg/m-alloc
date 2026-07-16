# m-alloc

[![CI](https://github.com/dornberg/m-alloc/actions/workflows/ci.yml/badge.svg)](https://github.com/dornberg/m-alloc/actions/workflows/ci.yml)
[![CodeQL](https://github.com/dornberg/m-alloc/actions/workflows/codeql.yml/badge.svg)](https://github.com/dornberg/m-alloc/actions/workflows/codeql.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![Platforms](https://img.shields.io/badge/platforms-Linux%20%7C%20Windows%20%7C%20macOS-lightgrey.svg)](#building)
[![Release](https://img.shields.io/github/v/release/dornberg/m-alloc?include_prereleases)](https://github.com/dornberg/m-alloc/releases)

A modern, high-performance, cross-platform memory allocator library written in C++23.

## Overview

m-alloc is a general-purpose memory allocator built as a clean, object-oriented C++23 library. It acquires memory directly from the operating system (`mmap` on Linux/macOS, `VirtualAlloc` on Windows) and manages it through a tiered architecture with size classes, segregated free lists, coalescing, and optional thread-local caches.

The project doubles as a reference implementation for allocator construction: every subsystem (OS memory layer, size classes, pools, boundary-tag heap, debug layer, statistics) is a small, testable class with a single responsibility.

## Features

- Three-tier allocation strategy: pooled size classes, coalescing segment heap, direct OS mappings
- 30 size classes from 8 B to 4 KiB with a constant-time lookup table
- Block splitting and immediate coalescing in the medium tier
- Aligned allocation for any power-of-two alignment
- `reallocate` with cross-tier migration
- Optional thread-local caches with batch refill and flush
- Fine-grained locking: one mutex per size class shard, one per tier
- Runtime statistics: counts, usage, peaks, internal and external fragmentation
- Optional debug layer: guard bytes, memory poisoning, double-free detection, invalid-pointer detection, corruption detection, leak tracking
- `std::expected`-based error reporting alongside a classic pointer API
- STL integration via `ma::StlAllocator<T>` and `std::pmr::memory_resource`
- Zero compiler warnings with GCC, Clang and MSVC (`-Wall -Wextra -Wpedantic -Werror` / `/W4 /WX`)

## Motivation

General-purpose allocators trade off between throughput, fragmentation, memory footprint and observability. m-alloc favors:

- Predictable behavior - explicit size classes and documented memory layout
- Observability - first-class statistics and debug instrumentation instead of external tooling
- Clean architecture - small classes behind a stable facade, easy to study and extend
- Portability - one code path per platform concern, everything else shared

## Architecture

```
                 +---------------------+
                 |    ma::Allocator    |   public facade (pimpl)
                 +----------+----------+
                            |
                 +----------v----------+
                 |    AllocatorImpl    |   routing, shims, stats, debug
                 +--+------+------+---++
                    |      |      |   |
            +-------v-+ +--v---+ +v---v-----+
            | Pool    | | Seg  | | Large    |
            | (small) | | Heap | | (direct) |
            +----+----+ +--+---+ +----+-----+
                 |         |          |
                 +----+----+----------+
                      |
              +-------v--------+
              | SystemMemory   |   mmap / VirtualAlloc
              +----------------+
```

| Tier | Request size | Strategy |
|---|---|---|
| Small | <= 4 KiB | Size-class pools, intrusive free lists, optional thread cache |
| Medium | 4 KiB - 256 KiB | 4 MiB segments, boundary tags, best-fit bins, split + coalesce |
| Large | > 256 KiB | Dedicated OS mapping per allocation |

See [docs/architecture.md](docs/architecture.md) for details.

## Memory layout

Every user pointer is preceded by a 16-byte header:

```
+----------------+--------+--------+----------+----------------------+
| payload size   | tier   | state  | offset   | user data ...        |
| 8 bytes        | 2 B    | 2 B    | 4 B      | 16-byte aligned      |
+----------------+--------+--------+----------+----------------------+
```

Medium-tier blocks additionally carry boundary tags (size, previous size, flags) enabling O(1) coalescing with both neighbors. See [docs/memory-layout.md](docs/memory-layout.md).

## Building

Requirements: CMake >= 3.25 and a C++23 compiler (GCC 13+, Clang 17+, MSVC 19.38+).

```sh
git clone https://github.com/dornberg/m-alloc.git
cd m-alloc
cmake --preset release
cmake --build --preset release
ctest --preset release
```

Presets: `debug`, `release`, `asan` (sanitizers), `msvc` (Windows).

CMake options:

| Option | Default | Description |
|---|---|---|
| `MALLOC_BUILD_TESTS` | `ON` (top level) | Build GoogleTest suite |
| `MALLOC_BUILD_BENCHMARKS` | `ON` (top level) | Build benchmark harness |
| `MALLOC_BUILD_EXAMPLES` | `ON` (top level) | Build examples |
| `MALLOC_ENABLE_SANITIZERS` | `OFF` | ASan + UBSan |
| `MALLOC_WARNINGS_AS_ERRORS` | `ON` | `-Werror` / `/WX` |

## Installation

```sh
cmake --preset release
cmake --build --preset release
cmake --install build/release --prefix /usr/local
```

Consume from CMake:

```cmake
find_package(MAlloc REQUIRED)
target_link_libraries(app PRIVATE MAlloc::MAlloc)
```

Or vendor it with `FetchContent`:

```cmake
FetchContent_Declare(malloc_lib
    GIT_REPOSITORY https://github.com/dornberg/m-alloc.git
    GIT_TAG v0.1.0)
FetchContent_MakeAvailable(malloc_lib)
```

## Usage

```cpp
#include <MAlloc/MAlloc.hpp>

int main() {
    ma::Allocator allocator;

    void* ptr = allocator.allocate(256);
    ptr = allocator.reallocate(ptr, 1024);
    void* aligned = allocator.alignedAllocate(64, 512);

    allocator.deallocate(aligned);
    allocator.deallocate(ptr);

    auto result = allocator.tryAllocate(128);
    if (!result) {
        return static_cast<int>(result.error());
    }
    allocator.deallocate(*result);
}
```

Debug mode:

```cpp
ma::Allocator allocator(ma::AllocatorConfig::debug());
void* p = allocator.allocate(64);
allocator.deallocate(p);
allocator.deallocate(p);   // reported as ma::Error::DoubleFree
```

STL containers:

```cpp
ma::Allocator allocator;
std::vector<int, ma::StlAllocator<int>> v{ma::StlAllocator<int>(allocator)};

ma::MemoryResource resource(allocator);
std::pmr::vector<std::pmr::string> names{&resource};
```

More in [examples/](examples/).

## Performance

Run the harness yourself - numbers depend heavily on hardware and workload:

```sh
cmake --preset release
cmake --build --preset release
./build/release/benchmarks/malloc_benchmarks
```

The suite measures allocation latency, deallocation latency, single- and multi-threaded throughput, peak RSS, and fragmentation against `std::malloc`, `std::pmr::synchronized_pool_resource`, and (when installed) mimalloc and jemalloc. Methodology and sample results in [docs/benchmarks.md](docs/benchmarks.md).

## Statistics

```cpp
const ma::StatisticsSnapshot stats = allocator.statistics();
// allocationCount, freeCount, activeAllocations,
// currentUsage, peakUsage, allocatedBytes, freedBytes,
// reservedBytes, internalFragmentation, externalFragmentation
```

## Roadmap

- [ ] In-place `reallocate` fast path for medium and large tiers
- [ ] Per-thread statistics
- [ ] NUMA-aware segment placement
- [ ] Decommit of long-idle segments (`madvise` / `VirtualFree(MEM_DECOMMIT)`)
- [ ] Lock-free central free lists for hot size classes
- [ ] `malloc`/`free` drop-in replacement shim

## Documentation

- [docs/allocator.md](docs/allocator.md) - public API reference
- [docs/architecture.md](docs/architecture.md) - internal design
- [docs/memory-layout.md](docs/memory-layout.md) - headers, blocks, segments
- [docs/benchmarks.md](docs/benchmarks.md) - methodology and results
- [docs/debugging.md](docs/debugging.md) - debug features and diagnostics

## Contributing

Contributions are welcome. Please read [CONTRIBUTING.md](CONTRIBUTING.md) and the [Code of Conduct](CODE_OF_CONDUCT.md) first. The project follows [Semantic Versioning](https://semver.org) and [Conventional Commits](https://www.conventionalcommits.org).

## License

MIT - see [LICENSE](LICENSE).
