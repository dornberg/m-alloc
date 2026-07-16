# Memory layout

## Universal block header

Every user pointer `p` is preceded by a 16-byte header at `p - 16`:

```
offset  size  field         meaning
0       8     payloadSize   bytes requested by the caller
8       2     tier          0x51 small, 0x52 medium, 0x53 large,
                            0x54 aligned shim, 0x55 debug shim
10      2     state         0xA11C allocated, 0xF4EE freed
12      4     offset        shim distance back to the underlying block
```

The `state` magic drives double-free and invalid-pointer detection. The header guarantees 16-byte alignment of every user pointer.

## Small tier

Runs of 64 KiB are carved into fixed-stride blocks:

```
run:   [ block | block | block | ... ]
block: [ header 16B | payload (class size) | padding to 16B stride ]
```

Free blocks store a single `next` pointer in their first 8 bytes (the header's `payloadSize` field), leaving `tier` and `state` intact so a freed block is still recognizable.

## Medium tier

4 MiB segments hold a sequence of boundary-tagged blocks:

```
segment: [ block ][ block ][ block ] ... to segment end

block:   +-------------------+------------------------------------+
         | MediumBlock 16B   | payload                            |
         |  sizeAndFlags 8B  |  [ universal header 16B | user ]   |
         |  prevSize     8B  |                                    |
         +-------------------+------------------------------------+
```

`sizeAndFlags` packs the total block size (always a multiple of 16) with three flag bits: free, first-in-segment, last-in-segment. `prevSize` makes the left neighbor reachable without scanning, so coalescing is O(1) in both directions.

Free medium blocks embed a doubly-linked list node 32 bytes into the block (past both headers), which keeps the universal header's `state` field intact for double-free detection. Minimum block size is 48 bytes.

## Large tier

```
mapping: [ 16B reserved | universal header 16B | user data ... | page padding ]
```

Each large allocation owns a page-rounded private mapping tracked in a registry keyed by the payload address.

## Aligned allocations

For alignment `A > 16`, the allocator requests `size + A` from the normal path and places the user pointer at the next `A` boundary. A shim header (tier `0x54`) at `user - 16` stores the distance back to the underlying block:

```
[ underlying header | ... gap ... | shim header 16B | user data (A-aligned) ]
                                                     ^ user
```

## Debug guard layout

With `enableGuardBytes`, the allocator wraps each block:

```
[ underlying header | guard 16B | shim header 16B | user data | guard 32B ]
  0xFB pattern -------^                                         ^-- 0xFB pattern
```

Guards are verified on free; any modified byte reports `Error::HeapCorruption`. Poisoning fills user data with `0xCD` on allocation and `0xDD` on free.
