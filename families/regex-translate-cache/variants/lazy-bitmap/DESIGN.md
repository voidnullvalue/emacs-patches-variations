# Design: Regex Translate Cache — Lazy Bitmap Variant
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline fills all 256 entries of `trt_ascii` at compile time by calling
`char_table_translate` 256 times. For patterns compiled frequently (e.g., in
a loop) or for patterns that only match a small subset of ASCII characters,
this upfront cost is wasteful.

This variant fills entries lazily: the bitmap is zeroed at compile time (32 bytes),
and each entry is filled only when first accessed by the match engine.

## Data Structures

`trt_ascii[256]` — cache array, same as baseline.
`trt_bitmap[32]` — 256-bit bitmap; bit I is set when `trt_ascii[I]` is valid.
`trt_bitmap_valid` — single flag: set when `translate` is non-nil and the lazy
cache is active.

## Hot-Path Macro

```c
#define TRANSLATE_TRU(C)                                          \
  ((bufp->trt_bitmap_valid && (unsigned)(C) < 128)               \
   ? (TRT_BIT_TEST(bufp, C)                                       \
      ? bufp->trt_ascii[(unsigned char)(C)]                       \
      : (TRT_BIT_FILL(bufp, C), bufp->trt_ascii[(unsigned char)(C)])) \
   : TRANSLATE(C))
```

On a hit (bit set): 1 flag check + 1 bit test + 1 array load.
On a miss (bit clear): fill entry + set bit + return value.
After warmup (all accessed characters filled), every call is the hit path.

## Compile-Time Cost

Baseline: 256 × `char_table_translate` calls + populate 256 bytes.
This variant: `memset(trt_bitmap, 0, 32)` — essentially free.

The trade-off: if all 256 ASCII characters are accessed during matching (rare for
most patterns), the total `char_table_translate` cost equals the baseline. If
only 26 characters (e.g., lowercase letters) are ever matched, only 26 fills
occur instead of 256.

## When This Variant Wins

- Patterns compiled many times (e.g., inside a loop) where each compilation
  is followed by a single match.
- Patterns that match text in a limited character set (only letters, only digits).

## When the Baseline Wins

- Long-running searches over diverse text where all 128 ASCII characters are
  eventually accessed: the lazy bitmap overhead adds an extra bit-test per call
  with no reduction in total fill cost.
