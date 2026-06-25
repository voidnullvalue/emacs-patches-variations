# Design: Malloc Pressure Relief — Freed-Byte Goal
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline always passes `goal=0` to `malloc_zone_pressure_relief`, meaning
"release as much as possible." The macOS allocator documentation also accepts
non-zero goals: passing `N` is a hint that the caller wants at least `N` bytes
released, potentially allowing the allocator to terminate the arena walk early.

This variant uses the number of bytes freed by the just-completed GC as the
goal. The intuition: if the GC freed 8 MB, we ask the allocator to return 8 MB
of free pages. The allocator may satisfy this early and stop walking, reducing
compaction latency on large heaps.

## Algorithm

```c
size_t goal = clamp(freed_bytes, 0, SIZE_MAX);
malloc_zone_pressure_relief(NULL, goal);
```

The trigger policy (threshold + periodic + rate-limit) is identical to the
baseline. The only difference is the goal argument.

## Integer Conversion Safety

`freed_bytes` has type `byte_ct` (alias for `uintmax_t`). Converting to `size_t`
requires care on platforms where `size_t` is 32-bit. The patch handles three cases:

1. `freed_bytes == 0` (periodic trigger, no freed data) → `goal = 0` (maximal).
2. `freed_bytes > SIZE_MAX` → `goal = SIZE_MAX` (clamp; in practice freed bytes
   cannot exceed physical RAM).
3. Otherwise → exact conversion.

## Semantic Difference from goal=0

| goal value | Semantics |
|---|---|
| 0 | Release as much as possible; walk entire arena |
| N > 0 | Release at least N bytes; may terminate early |

The allocator **may** release more than `goal` bytes regardless. The hint only
affects whether the walk terminates before surveying all arenas.

**Caveat:** The macOS allocator documentation does not guarantee early-termination
behavior for non-zero goals. This is an optimization hint. Benchmark against
`goal=0` before assuming latency improvement.

## Comparison Table

| Property | Baseline (goal=0) | Freed-byte goal |
|---|---|---|
| Arena walk depth | Full | Potentially partial |
| Trigger policy | Threshold + periodic | Same |
| Integer conversion | N/A | Safe (SIZE_MAX clamp) |
| Latency | Baseline | ≤ baseline (hint-dependent) |
