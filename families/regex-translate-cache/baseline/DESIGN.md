# Design: Regex Translate Cache Baseline
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Problem

Case-insensitive (`case-fold-search t`) regex searches use a translate table
stored in `re_pattern_buffer.translate`. The `TRANSLATE(d)` macro calls
`char_table_translate(translate, d)` on every character compared during matching.
This is a general char-table walk, expensive relative to the comparison itself.

For ASCII characters (the common case), the translation (typically lowercase→lowercase,
uppercase→lowercase) is constant for the life of the pattern.

## Approach

Add `trt_ascii[256]` (`unsigned char`) and `trt_ascii_valid` (`bool_bf`) to
`re_pattern_buffer`. At `re_compile_pattern` time, if `translate` is non-nil,
fill `trt_ascii` by calling `char_table_translate(translate, i)` for i = 0..255
and converting each result to a byte via `CHAR_TO_BYTE_SAFE`.

Replace `TRANSLATE()` and `RE_TRANSLATE()` calls in the hot match paths with
`TRANSLATE_TRU(C)`:

```c
#define TRANSLATE_TRU(C)                         \
  ((bufp->trt_ascii_valid && (unsigned)(C) < 128) \
   ? bufp->trt_ascii[(unsigned char)(C)]          \
   : TRANSLATE(C))
```

Note the guard `< 128` even though the table has 256 entries. Codes 128–255
have different semantics in multibyte vs unibyte mode (see code comments), so
they always take the slow path. The 256-entry array is filled for completeness
but only the first 128 entries are used via the fast path.

## Key Design Choice: unsigned char entries

Translation results are stored as `unsigned char`. For ASCII inputs, the
translation is always an ASCII character (e.g., uppercase → lowercase within
ASCII), so truncation to `unsigned char` is lossless.

For codes 128–255, `CHAR_TO_BYTE_SAFE` converts the Unicode codepoint result
to a raw byte value. This conversion may return `-1` for characters without a
direct byte representation; in that case the entry is set to `i` (identity) and
the fast path would give the wrong answer — hence the `< 128` guard that forces
codes ≥ 128 to the slow path regardless of `trt_ascii_valid`.

See `variants/wide-entry` for a 16-bit alternative that removes this constraint.

## Performance Notes

Original README reports up to 50% improvement in case-folding search scenarios.
The gain is highly sensitive to the fraction of ASCII characters in the searched
text and the depth of the match engine inner loops.
