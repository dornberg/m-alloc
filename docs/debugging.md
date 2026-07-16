# Debugging

## Enabling the debug layer

```cpp
ma::Allocator allocator(ma::AllocatorConfig::debug());
```

`AllocatorConfig::debug()` enables guard bytes, poisoning, leak tracking and double-free detection, and disables the thread cache so every operation is validated centrally. Each feature can also be toggled individually.

## Error reporting

Errors are delivered to `config.errorHandler`; without a handler they are printed to stderr. Set `abortOnError = true` to crash at the first violation (useful under a debugger or in CI).

```cpp
ma::AllocatorConfig config = ma::AllocatorConfig::debug();
config.errorHandler = [](ma::Error error, const void* ptr) {
    std::println(stderr, "allocator error: {} at {}", ma::toString(error), ptr);
};
config.abortOnError = true;
```

## Features

### Guard bytes

32-byte zones filled with `0xFB` surround each allocation. Any byte changed by an overflow or underflow is detected on free and reported as `Error::HeapCorruption`. The block is still released afterwards.

### Memory poisoning

- `0xCD` fills fresh allocations - reads of uninitialized memory become recognizable
- `0xDD` fills freed memory - use-after-free reads become recognizable

Seeing these patterns in a debugger immediately classifies the bug.

### Double-free detection

Every block header carries a state magic (`0xA11C` allocated, `0xF4EE` freed). Freeing a block whose state is already freed reports `Error::DoubleFree`. With leak tracking enabled, a tombstone registry makes this detection exact even after the block memory is reused.

### Invalid-pointer detection

Pointers that were never produced by the allocator fail header validation and report `Error::InvalidPointer`. With leak tracking enabled the check happens against the live registry before any header is dereferenced, which makes it safe even for wild pointers into unmapped memory.

### Leak detection

With `enableLeakTracking`, every live allocation is registered. `allocator.liveAllocationCount()` returns the current count, and destroying an allocator with live allocations prints a summary to stderr.

```cpp
{
    ma::Allocator allocator(ma::AllocatorConfig::debug());
    void* leaked = allocator.allocate(64);
}   // stderr: m-alloc: 1 allocation(s) leaked
```

## Limitations

- Guard bytes apply to default-aligned allocations; over-aligned allocations get poisoning and tracking but no guards
- Without leak tracking, invalid-pointer detection dereferences the candidate header, which is best-effort for pointers into unmapped memory (same caveat as any malloc implementation)
- Debug features cost time and memory; keep them out of release builds

## Working with sanitizers

The `asan` preset builds the library and tests with AddressSanitizer and UndefinedBehaviorSanitizer:

```sh
cmake --preset asan
cmake --build --preset asan
ctest --preset asan
```

Sanitizers and the debug layer are complementary: sanitizers catch bugs in code using the allocator, the debug layer catches misuse of the allocator API itself.
