# Design: Malloc Pressure Relief Baseline
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Problem

On macOS, Emacs is built with `SYSTEM_MALLOC`. When Lisp objects are freed during
GC, the memory is returned to the system malloc zone's free lists, not to the OS.
The allocator keeps those pages mapped, so resident set size (RSS) grows
monotonically across a long session even if the live Lisp heap is stable.

`malloc_zone_pressure_relief(NULL, 0)` (available since macOS 10.7) walks all
registered malloc zones and calls `munmap` on pages that are entirely free.
This is a documented, public API that does not require private entitlements.

## Approach

Add `maybe_compact_memory_after_gc(freed_bytes)` and call it at the very end of
`garbage_collect()`, after all sweeping and before `post-gc-hook`. The function
applies three gates before calling `malloc_zone_pressure_relief`:

1. **Threshold gate:** Only run if the GC freed ≥ 4 MB of live data, indicating
   there are likely returnable pages.
2. **Periodic gate:** Run anyway every 64 GCs so gradual fragmentation does not
   accumulate unchecked between large collections.
3. **Rate-limit gate:** Skip if fewer than 3 seconds have elapsed since the last
   compaction, preventing pathological churn on allocation-heavy workloads.

## Key Design Choices

**Synchronous, post-sweep placement.** Calling pressure relief after all sweeping
maximizes the chance of finding fully-free pages (freed objects are back on the
zone's free list). The cost is a latency spike on the GC that triggers compaction.
See `variants/background-dispatch` for the async alternative.

**NULL zone argument.** Passing NULL to `malloc_zone_pressure_relief` iterates
all registered zones, which is correct for Emacs's usage. An explicit zone loop
via `malloc_get_all_zones()` would add complexity without changing the result in
typical configurations.

**Static thresholds.** The constants `MEMORY_COMPACT_FREED_THRESHOLD` (4 MB),
`MEMORY_COMPACT_EVERY_N_GCS` (64), and `MEMORY_COMPACT_MIN_INTERVAL_SECS` (3)
are chosen conservatively. See `variants/adaptive-threshold` for learned thresholds.

**File-static statistics.** `memory_compacts_done` and
`memory_compact_bytes_returned` are file-static rather than `DEFVAR_INT` to
avoid touching the auto-generated `globals.h` table. They can be exposed later
if monitoring is needed.

## Limitations

`malloc_zone_pressure_relief` cannot compact live objects. A page with even one
live object stays resident. This means the patch does not help with internal
fragmentation — it only reclaims pages that are entirely empty after GC.
