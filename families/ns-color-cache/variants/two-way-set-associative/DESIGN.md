# Design: NSColor Cache — 2-Way Set-Associative
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline's direct-mapped cache has an Achilles heel: two distinct colors that
hash to the same slot thrash the cache indefinitely, yielding zero benefit for
those two colors regardless of how large the table is. A set-associative cache
gives each set multiple ways (slots), so two colliding keys can coexist. With
2 ways, a pair of colliding colors no longer thrashes; the CLOCK hand picks the
less-recently-used one when both ways are full.

This variant uses 256 sets × 2 ways = 512 total entries. Memory is statically
bounded at ~12 KB — much smaller than the baseline's ~786 KB, yet with better
worst-case behavior against adversarial color patterns.

## Cache Organization

- **Sets:** 256, indexed by the top 8 bits of the Fibonacci hash of the packed color.
- **Ways:** 2 per set — two entries share a set index.
- **Total entries:** 512.
- **Per-entry state:** `{NSColor*, unsigned long key, bool occupied, bool clock_ref}`.
- **Per-set CLOCK hand:** 1-bit integer (0 or 1), 256 bytes total.

## Replacement Policy

On a miss into a set:
1. If either way is unoccupied, use it.
2. Otherwise, apply CLOCK: check the hand position's `clock_ref`. If clear, evict
   that slot. If set, clear `clock_ref`, advance hand to the other way, and evict there.

With only 2 ways, the CLOCK degenerates to: "give each entry one second chance,
then evict the one that hasn't been referenced since the hand last passed."

## Hash-to-Set Mapping

`set = (key * 11400714819323198485ULL) >> 56`

Top 8 bits of the 64-bit Fibonacci hash provide a uniform distribution over the
256 sets. This is consistent with the baseline's use of Fibonacci hashing.

## Collision Behavior

Two colors hashing to the same set coexist if both are in their two ways. A third
distinct color with the same set index will evict one of the two existing entries
via CLOCK. This is the expected and documented behavior for a fixed-size cache.

## Retain/Release Discipline

On eviction, `[evicted.color release]` is called before storing the new entry.
On insertion, `[col retain]` is called. The cache owns one reference to each
stored NSColor for its entire lifetime in the cache.

## Comparison Table

| Property | Baseline | 2-way set-associative | CLOCK-LRU (64) |
|---|---|---|---|
| Total entries | 32768 | 512 | 64 |
| Memory | ~786 KB | ~12 KB | ~1.5 KB |
| Eviction | Never | CLOCK per set | CLOCK global |
| Collision resistance | Low (direct-mapped) | Medium (2 ways) | N/A (linear scan) |
| Lookup cost | O(1) | O(1) — 2 comparisons | O(N) linear scan |
