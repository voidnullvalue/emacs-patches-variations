# Design: NSColor Cache — Single-Entry Hot Cache
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline allocates ~786 KB for a 32768-slot direct-mapped table that is
rarely more than 1% populated during typical sessions. The CLOCK-LRU variant
uses 64 slots. This variant goes further: allocate nothing beyond three static
variables and exploit the strongest possible temporal locality assumption — that
the most recently requested color is the most likely to be requested again.

This assumption holds when the NS drawing path renders long runs of text in the
same face color (e.g., a line of black text). In that case, consecutive calls to
`+colorWithUnsignedLong:` all carry the same packed value, and the single cached
entry hits on every call after the first.

It fails when the drawing path alternates between two or more colors on every
call, in which case every call misses and replaces the entry, providing no benefit
beyond one extra comparison.

## Algorithm

**State:** `ns_hot_color` (NSColor*), `ns_hot_key` (unsigned long), `ns_hot_valid` (bool).

**Lookup:** If `ns_hot_valid && ns_hot_key == key`, return `ns_hot_color`. Else nil.

**Store:** Release `ns_hot_color` if `ns_hot_valid`, then retain and store `col` as the new
`ns_hot_color`. Set `ns_hot_key = key`, `ns_hot_valid = true`.

**Teardown:** Call `ns_hot_cache_teardown()` from `ns_term_shutdown`. This releases
the retained object and clears the validity flag.

## Retain/Release Discipline

The cached NSColor is retained when stored (`[col retain]`). On replacement, the
old object is released before retaining the new one. On teardown the object is
released. This correctly tracks the cache's ownership of the NSColor across its
lifetime, preventing both leaks and use-after-free.

## Workload Characterization

| Workload | Expected hit rate |
|---|---|
| Long run of same-face text | ~100% after first call |
| Alternating foreground/background | ~50% (two-color ping-pong) |
| Rendering many distinct faces per line | ~0% (random miss each call) |
| Cursor blink (changes one color) | ~50% |

## Comparison with Other Variants

| Property | Baseline | Single-entry hot cache | CLOCK-LRU (64) |
|---|---|---|---|
| Memory | ~786 KB | 24 bytes | ~1.5 KB |
| Hit rate (2 colors) | High (32768 slots) | 50% | High (64 slots) |
| Hit rate (60 colors) | High | ~0% | ~100% |
| Eviction | Never | Unconditional replace | CLOCK |
| Teardown needed | No | Yes | No |
