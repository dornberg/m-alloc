# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.0] - 2026-07-16

### Added

- Tiered allocator core: size-class pools (<= 4 KiB), boundary-tag segment heap with splitting and coalescing (4 KiB - 256 KiB), direct OS mappings (> 256 KiB)
- OS memory layer using `mmap` on Linux/macOS and `VirtualAlloc` on Windows
- Public API: `allocate`, `deallocate`, `reallocate`, `alignedAllocate` plus `std::expected`-based `try*` variants
- Optional thread-local caches with batch refill and flush
- Debug layer: guard bytes, memory poisoning, double-free detection, invalid-pointer detection, corruption detection, leak tracking
- Runtime statistics: allocation/free counts, current and peak usage, allocated/freed bytes, internal and external fragmentation
- STL integration: `ma::StlAllocator<T>` and `std::pmr::memory_resource` adapter
- GoogleTest suite covering small/large allocations, alignment, invalid and double frees, stress, random workloads, thread safety, leak detection and fragmentation
- Benchmark harness comparing std::malloc, std::pmr, mimalloc and jemalloc
- Examples: basic allocation, placement new, STL integration, custom configuration, benchmark, statistics
- CMake package config, presets and install rules
- Documentation: API reference, architecture, memory layout, benchmarks, debugging

[Unreleased]: https://github.com/dornberg/m-alloc/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/dornberg/m-alloc/releases/tag/v0.1.0
