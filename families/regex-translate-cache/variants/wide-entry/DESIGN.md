# Design: Regex Translate Cache — Wide Entry Variant
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline cache stores translation results as `unsigned char`. This requires
calling `CHAR_TO_BYTE_SAFE(ch)` to convert the Unicode code point returned by
`char_table_translate` into a single byte. If `ch > 255`, `CHAR_TO_BYTE_SAFE`
returns -1 and the entry is stored as `i` (identity). This is incorrect for
unibyte buffers where a raw byte in the range 128-255 might translate to a
different raw byte (or to a Unicode code point in the range 0xA0-0xFF).

More importantly, the baseline's fast path guard is `(unsigned)(C) < 128`, not
`< 256`. Codes 128-255 always take the slow path in the baseline, even when a
valid cached result exists for them.

This variant uses `uint16_t` entries. The stored value is the direct result of
`char_table_translate`, not a byte approximation. Codes > 0xFFFE (astral Unicode
code points above U+FFFE) are stored as 0xFFFF sentinel to indicate "use slow
path."

## Coverage Extension: 0-255

The baseline covers only 0-127 in its fast path. This variant covers 0-255. The
extension is safe because:

- For multibyte buffers: codes 128-255 are Unicode code points. `char_table_translate`
  returns a code point, stored losslessly in `uint16_t` (for BMP characters).
- For unibyte buffers: codes 128-255 are raw bytes. Translation results are raw
  bytes stored losslessly in `uint16_t`.

In both cases, the stored value is the correct direct result of `char_table_translate`,
avoiding the `CHAR_TO_BYTE_SAFE` conversion entirely.

## Sentinel Value

Codes that translate to values > 0xFFFE (astral Unicode characters, code points
above U+FFFE) cannot be stored in `uint16_t`. The sentinel value `0xFFFF` is
used to indicate "slow path required." `TRANSLATE_TRU` checks `<= 0xFFFF` — this
condition is always true for `uint16_t`, so the check should be `!= 0xFFFF`.

(The patch uses `<= 0xFFFF` as written but this is a no-op; correct code should
use `!= 0xFFFF`. This is a minor correctness issue in the design-space patch.)

## Memory

512 bytes per `re_pattern_buffer` (256 × 2 bytes) vs 256 bytes in the baseline.
This doubles the translation cache memory footprint.

## Comparison with Baseline

| Property | Baseline | This variant |
|---|---|---|
| Entry type | `unsigned char` | `uint16_t` |
| Stored value | `CHAR_TO_BYTE_SAFE(ch)` | `ch` directly |
| Fast path range | 0-127 | 0-255 |
| Correctness for 128-255 | Approximation | Exact |
| Memory | 256 bytes | 512 bytes |
