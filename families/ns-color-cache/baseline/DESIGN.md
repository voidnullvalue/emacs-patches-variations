# Design: NSColor Cache Baseline
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Problem

`+colorWithUnsignedLong:` is called for every glyph drawn in the NS port. It
allocates a new `NSColor`, performs floating-point ARGB unpacking, and calls
`colorForEmacsRed:green:blue:alpha:`, which internally performs colorspace
conversion. The result is immediately autoreleased.

Because Emacs faces use a small, bounded set of colors (typically < 30 for a
dark theme), the same pixel values are looked up thousands of times per redraw
with identical results each time.

## Approach

Interpose a direct-mapped cache between `+colorWithUnsignedLong:` and the
underlying `colorForEmacsRed:` factory. The cache is a pair of parallel arrays
(`ns_color_cache` and `ns_color_cache_keys`), indexed by a Fibonacci hash of the
packed pixel value.

On a hit the cached `NSColor*` is returned directly. On a miss the color is
created normally and stored in the slot (retaining it) if the slot is empty.
If the slot holds a different key (hash collision), the freshly-created
autoreleased color is returned without storing it, so no prior entry is
displaced.

## Key Design Choices

**No eviction.** Emacs's face color set is stable across most of a session; the
handful of colors loaded at startup remain in use until the frame is closed.
Eviction would add overhead for zero practical benefit.

**Fibonacci hash.** Packed ARGB values cluster in low bytes (alpha is almost
always 0xFF). A multiplicative hash spreads these across the 32768-slot table
without extra computation.

**Retained NSColor.** The cache holds a strong reference (`retain`) to each
stored object. This prevents the autorelease pool from reclaiming them between
draw events, which is the correctness requirement for a process-lifetime cache.

**No locking.** `+colorWithUnsignedLong:` is only called from NS drawing code
which always runs on the main thread.

## Size Rationale

32768 slots = 2^15. At 16 bytes per `NSColor*` + 8 bytes per `unsigned long` key
(aligned to 8), the two arrays occupy ~786 KB. This is a fixed cost regardless
of how many distinct colors are actually used. For most sessions the live set is
< 64 colors, so > 99.8% of slots are unused but still warm in the page table.

A smaller table (see `variants/counted-lru`) trades this for tighter memory
footprint at the cost of more complex eviction.
