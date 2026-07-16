# Third-party dependencies

m-alloc has no runtime dependencies. Everything below is used only for development and is fetched or discovered at configure time - nothing is vendored into this directory.

| Dependency | Purpose | Acquisition |
|---|---|---|
| GoogleTest v1.15.2 | Unit tests | CMake `FetchContent` (tests only) |
| mimalloc | Benchmark comparison | `find_package`, optional |
| jemalloc | Benchmark comparison | `find_library`, optional |

If a future dependency ever needs vendoring, it lands here with its own license file.
