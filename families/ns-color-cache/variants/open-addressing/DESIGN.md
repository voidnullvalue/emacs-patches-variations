# Design: NSColor Cache — Open-Addressing Variant
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline's direct-mapped cache silently misses whenever two pixel values
hash to the same slot — a common event when themes use many colors with similar
high bytes. This variant resolves collisions by probing forward up to 8 slots.

## Algorithm

Each slot is a `NsColorEntry` struct: `{NSColor*, unsigned long key, uintmax_t gen}`.
A global generation counter `ns_color_gen` is incremented on every insertion.

**Lookup:** Hash the key, probe linearly up to `NS_COLOR_CACHE_PROBE_MAX = 8` slots.
Return on match; stop on empty slot (key is absent).

**Insertion:** Same probe walk. Use first empty slot. If all 8 slots are occupied
by distinct keys, overwrite the slot with the smallest `gen` value (approximately
least recently inserted). Release the evicted NSColor.

## Why 512 Slots / 8 Probes

512 slots × 8-probe window means a probe covers a contiguous 8-entry span
(128 bytes on arm64, 2 cache lines). This is cache-friendly. At typical Emacs
face counts (< 64 colors), load factor stays well below 0.125 (64/512), so
almost all lookups find the key in the first 1–2 probes.

## Eviction

When the probe window is full, the entry with the lowest generation stamp is
evicted. This is an approximate LRU: because insertions increment `ns_color_gen`,
the entry with the smallest stamp was inserted longest ago. The evicted NSColor
is released, allowing it to be deallocated if no other reference exists.

## Comparison with Baseline

| Property | Baseline | This variant |
|---|---|---|
| Table size | 32768 slots | 512 slots |
| Memory | ~786 KB | ~16 KB |
| Collision handling | Silent miss | Linear probe |
| Eviction | Never | Generation-based |
| Lookup (hit, no collision) | 1 step | 1 step |
| Lookup (hit, k collisions) | Miss | k+1 steps |

## Correctness Note

The `[release]` on eviction is safe because the Emacs color lifecycle is:
1. `+colorWithUnsignedLong:` returns a color (retained by cache or autoreleased)
2. Caller uses the color for a draw call (synchronous, on the main thread)
3. At frame end, the autorelease pool drains

Eviction only releases the cache's own retain. If the color was already retained
by another caller (unusual), the release decrements the count without deallocation.
The evicted slot then holds a new color, so subsequent cache hits return the new
color, not the evicted one.
