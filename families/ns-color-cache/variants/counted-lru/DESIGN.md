# Design: NSColor Cache — CLOCK-LRU Variant
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline allocates ~786 KB for its 32768-slot table regardless of how many
colors are actually used. For a session with 30 distinct colors, 32738 slots
are wasted. The open-addressing variant reduces this to ~16 KB but still uses
a hash table with a fixed-size allocation.

This variant uses a tiny 64-slot array with CLOCK-approximation LRU eviction.
It is the right choice when:
- Memory budget is tight (embedded systems, memory-constrained builds)
- The active color count is known to be < 64
- Approximate LRU ordering is more important than O(1) lookup

## Algorithm

**Structure:** `NsColorLRUEntry[64]` with fields `{NSColor*, unsigned long key, bool ref, bool occupied}`.

**Lookup:** Linear scan over all 64 entries. If key matches, set `ref = true` and return color.

**Eviction (CLOCK):**
- If slots available, return first unoccupied slot.
- Otherwise: advance `ns_lru_hand`. If current slot's `ref == false`, evict it.
  If `ref == true`, clear `ref` and advance. Repeat.

The CLOCK algorithm guarantees that a slot is only evicted if its `ref` bit has
been clear for at least one full pass of the clock hand. Frequently-referenced
colors have `ref` set on every hit, protecting them from eviction.

## Why O(N) Lookup Is Acceptable

With N=64 and `NsColorLRUEntry` at ~24 bytes, the full table is 1.5 KB — one
or two cache lines on modern hardware after warming. A linear scan of 64 entries
is ~64 comparisons, which on modern CPUs executes in roughly 10–20 ns (comparable
to the Fibonacci hash + array load of the baseline, which may cause a cache miss
if the 786 KB table is not hot).

In practice, most hits will occur in the first few slots if colors are used in
frequency order (theme's background color first, etc.). The scan short-circuits
on the first match.

## Size Rationale

64 slots is chosen to exceed the typical active color count for any Emacs theme
(typically 10–50). If profiling shows more than 64 active simultaneous colors,
increase `NS_COLOR_LRU_SIZE` to the next power of 2.
