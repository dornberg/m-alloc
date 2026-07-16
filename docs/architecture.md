# Architecture

## Design goals

- Single-responsibility classes composed behind a stable facade
- Dependency inversion at the OS boundary so higher tiers never issue syscalls directly
- RAII ownership of every mapping - no manual unmap paths
- No global mutable state: each `ma::Allocator` is a fully independent heap

## Layer diagram

```
public   ma::Allocator, ma::StlAllocator<T>, ma::MemoryResource
            |
core     detail::AllocatorImpl      routing, shims, validation, statistics
            |
tiers    detail::PoolAllocator      small: size-class pools
         detail::SegmentHeap        medium: boundary-tag heap
         detail::LargeAllocator     large: direct mappings
            |
support  detail::ThreadCache        per-thread block caches
         detail::DebugLayer         guards, poisoning, leak registry
         detail::StatisticsCollector atomic counters
            |
os       detail::SystemMemory       mmap / VirtualAlloc behind one class
         detail::MappedRegion       RAII wrapper for a mapping
```

## Request routing

`AllocatorImpl` classifies each request by size:

| Condition | Path |
|---|---|
| `size <= 4096` | `PoolAllocator`, optionally through `ThreadCache` |
| `size <= 262144` | `SegmentHeap` |
| otherwise | `LargeAllocator` |

Guard bytes and aligned allocations are implemented as shims: the underlying block is obtained from the normal path, and a secondary header immediately before the user pointer records how to recover the original block on free.

## Small tier

`PoolAllocator` maintains 30 size classes (8 B to 4096 B). Each class is an independent shard with its own mutex, an intrusive free list, and a bump region carved from 64 KiB runs mapped on demand. Fixed block sizes eliminate external fragmentation inside a class; the class table bounds internal fragmentation to roughly 12 percent worst case.

## Medium tier

`SegmentHeap` maps 4 MiB segments and manages them with boundary tags. Every block stores its own size and the previous block's size, so both neighbors are reachable in O(1). Free blocks are kept in 18 power-of-two bins with doubly-linked nodes embedded in the free block itself. Allocation is first-fit within the matching bin, splitting the block when the remainder is at least 48 bytes. Freeing coalesces immediately with both neighbors. A segment whose single block spans the whole segment is returned to the OS (one segment stays resident to absorb churn).

## Large tier

`LargeAllocator` gives each allocation a dedicated page-rounded mapping registered in a hash map, which also provides pointer validation. Freeing unmaps immediately, returning memory to the OS.

## Thread caching

When enabled, each thread lazily builds a `ThreadCache` per allocator instance: a small array of blocks per size class, refilled in batches of 16 from the shared pools and flushed in batches when over capacity. Caches hold a `weak_ptr` to the central state, so a cache outliving its allocator flushes into nothing and never touches freed memory. Cache destruction on thread exit returns all blocks to the shared pools.

## Locking model

- One `std::mutex` per size class shard (30 independent locks)
- One mutex for the segment heap
- One mutex for the large-allocation registry
- One mutex for the leak registry (debug only)
- Statistics use relaxed atomics, no locks

Small allocations served from a thread cache take no lock at all.

## Error handling

Internal operations return `std::expected<T, ma::Error>`. The facade converts to pointers for the classic API and forwards errors to the configured handler. `abortOnError` upgrades reports to `std::abort()` for fail-fast environments.
