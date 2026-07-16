# Benchmarks

## Running

```sh
cmake --preset release
cmake --build --preset release
./build/release/benchmarks/malloc_benchmarks [iterations]
```

Default is 500000 iterations per scenario. mimalloc and jemalloc are picked up automatically when development packages are installed; otherwise those rows are skipped.

## Scenarios

| Scenario | What it measures |
|---|---|
| Allocation latency | ns per 64 B allocation, hot loop |
| Deallocation latency | ns per free of those blocks |
| Single-thread throughput | Mops/s over mixed 16 B - 8 KiB sizes with periodic batch frees |
| Multi-thread throughput | Same workload across `hardware_concurrency / 2` threads |
| Peak memory | Peak RSS after the scenarios (getrusage / GetProcessMemoryInfo) |
| Fragmentation | m-alloc internal and external fragmentation under churn |

## Sample results

Linux x86-64, GCC 16, release build, 100000 iterations, one representative run:

| allocator | alloc ns/op | free ns/op | 1T Mops/s | MT Mops/s |
|---|---|---|---|---|
| std::malloc (glibc) | 48.1 | 24.3 | 24.0 | 73.0 |
| std::pmr::synchronized_pool_resource | 21.9 | 26.9 | 7.3 | 6.2 |
| m-alloc | 74.4 | 48.9 | 12.3 | 7.7 |

Treat these as an honest baseline, not a marketing table: glibc malloc is extremely mature, and m-alloc v0.1 pays for statistics accounting and validation on every operation. The roadmap items (lock-free central lists, in-place realloc) target exactly the gaps visible here.

## Methodology notes

- `std::chrono::steady_clock`, one warm process per run
- Each adapter runs the identical workload behind a virtual interface; the dispatch cost is equal for all candidates
- Peak RSS is process-wide and cumulative across scenarios, so compare trends, not absolutes
- Run several times and compare medians; single runs jitter a few percent
