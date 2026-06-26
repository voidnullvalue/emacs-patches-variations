# LOCAL-PERF-004: Per-Call NSColor Allocation in +colorWithUnsignedLong:
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
     Human review status: not-reviewed -->

## 1. Precise Description

In `src/nsterm.m`, the category method `+colorWithUnsignedLong:(unsigned long)c`
is called on every glyph drawn by the Emacs NS/macOS display back-end. For each
call it:

1. Decomposes the packed pixel value `c` into ARGB floating-point components
2. Calls `[NSColor colorForEmacsRed:green:blue:alpha:]` which allocates a new
   `NSColor` object in the autorelease pool
3. Performs colorspace conversion internally within `NSColor`

Since Emacs faces use a small, bounded set of colors (typically < 100 distinct
values per session), the same color is derived dozens to millions of times during
a drawing-intensive session. Each call allocates a new object and performs an
identical colorspace conversion despite the result being identical to previous
calls.

The call chain appears in the hot path of `nsfont_draw` and `ns_draw_glyph_string`.
On color-heavy buffers (syntax-highlighted source, mixed faces) this contributes
measurable latency to each redisplay cycle.

**Platform:** macOS only (Objective-C, NSColor, NS display back-end).

## 2. Exact Affected Emacs Commit / Tag

- **Base commit:** `3ca168b80ae6d7b25fe55784dde3ad24faff7be2` (2026-06-23)
- **Nearest tag:** `emacs-31.0.90` (350 commits earlier)
- **Blob hash (verified):**
  - `src/nsterm.m`: `a7f3fc292c7fd87a067344008b7afad5ddcaa1ad`

## 3. Deterministic Reproduction

**On macOS:**
```sh
sh issues/LOCAL-PERF-004/reproduce.sh
```

The script:
1. Builds the patched and unpatched Emacs (requires macOS SDK, Xcode tools, autoconf)
2. Runs a benchmark that repeatedly redraws a syntax-highlighted buffer
3. Records frame-render time before and after the patch

**On Linux:** BLOCKED — requires macOS and Objective-C compiler.

The pure-C cache logic test (`test_cache_logic.c`) runs on Linux and verifies
the Fibonacci hash function and array lookup behavior.

## 4. Baseline Measurements / Failing Behavior

**Expected (macOS, not measured on this host):**
- Each `+colorWithUnsignedLong:` call: ~50–200 ns (allocation + conversion)
- A redisplay of 1000-glyph buffer with 10 face colors: ~10 000–200 000 calls
- Warm-cache miss rate: ~0% after first redisplay of a buffer with stable faces

**On this Linux host:** BLOCKED. No macOS SDK or ObjC compiler available.

The cache-logic C test (`tests/test_cache_logic.c`) runs and verifies:
- Fibonacci hash distributes keys uniformly
- Cache returns nil on miss (nil-initialized array)
- Cache returns the stored object on hit
- Collision keys do NOT overwrite existing entries (no-evict policy)

## 5. Candidate Fix

**Primary:** `families/ns-color-cache/baseline/implementation.patch`

Adds a 32768-slot direct-mapped C array cache (`NSColor *ns_color_cache[32768]`)
keyed by Fibonacci-hashed pixel value. On cache hit, returns the retained
`NSColor` object directly without allocation or colorspace conversion.

The `git apply --check` against the base commit passes (verified on Linux).

**Alternative (SPECIFICATION):**
`families/ns-color-cache/variants/nscache/implementation.patch`
— Uses `NSCache` with advisory `countLimit=512`. Not a real applicable patch.

## 6. Regression Test

`issues/LOCAL-PERF-004/tests/test_cache_logic.c`

This test covers the pure-C cache logic (hash function, array operations) and
runs on Linux. It does NOT test NSColor behavior (macOS only).

```sh
cc -O2 -o /tmp/t4 issues/LOCAL-PERF-004/tests/test_cache_logic.c && /tmp/t4
```

The full regression test (NSColor allocation count before/after) is blocked on Linux.

## 7. Apply, Build, Test, Benchmark Status

| Step | Status | Notes |
|---|---|---|
| `git apply --check` | **passed** | Verified on Linux against base commit |
| `./autogen.sh` | blocked | No autoconf (Linux) |
| `./configure --with-ns` | blocked | No macOS SDK (Linux) |
| `make -jN` | blocked | macOS-only |
| Cache logic test (C) | **passed** | Hash + array logic verified on Linux |
| NSColor allocation test | blocked | macOS only |
| Frame-render benchmark | blocked | macOS only |

## 8. Remaining Risks and Limitations

1. **Platform-blocked:** All Objective-C building and testing requires macOS.

2. **Process-lifetime cache / no teardown:** The 32768-slot array is never freed.
   On multi-display setups, colors from a disconnected display remain in the cache
   (occupying slots but pointing to now-invalid contexts). The `per-display-backend`
   variant addresses this.

3. **Hash collision:** On rare color distributions that produce many Fibonacci
   hash collisions for a 32768-slot table, some colors will always miss the cache.
   This is a correctness-safe miss (falls through to allocation), not a bug.

4. **No reference counting:** The retained `NSColor` objects are never released.
   On macOS this is acceptable (process lifetime), but it precludes per-frame
   cache clearing in future.

5. **LLM-assisted:** All patches and tests are LLM-generated.
