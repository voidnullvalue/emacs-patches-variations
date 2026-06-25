# Design: Regex Syntax Cache — 256-Entry (Single-Byte Full Range)
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline caches syntax codes for codes 0–127 only. In unibyte buffers, byte
codes 128–255 represent real characters with non-trivial syntax codes (e.g., in
Latin-1 encoded buffers). The baseline's `SYNTAX_TRU` falls back to
`SYNTAX(c)` for all such codes, adding a char-table walk for every
word-boundary opcode involving a high byte.

Extending the cache to 256 entries eliminates the slow path for all byte codes,
not just ASCII.

## Memory Impact

- Baseline: `unsigned char syntax_ascii[128]` — 128 bytes in `re_pattern_buffer`.
- This variant: `unsigned char syntax_ascii[256]` — 256 bytes in `re_pattern_buffer`.
- Increase: 128 bytes per compiled pattern.

For a session with 100 simultaneously active compiled patterns, the increase is
12.8 KB — negligible for most builds.

## Correctness in Multibyte Buffers

In multibyte buffers, byte codes 128–255 appear as the first (leading) byte of
multibyte sequences. The word-boundary opcode extracts full character codes
(via `GET_CHAR_BEFORE_2`, `GET_CHAR_AFTER`) before calling `SYNTAX`. These
full character codes are typically > 255 and fall outside the cache range,
so the extended cache does not affect them. The fast path only fires when the
value passed to `SYNTAX_TRU` is literally < 256, which is true for unibyte
characters and false for multibyte code points above 255.

## Population

At compile time: `syntax_property(i, 0)` for `i = 0, ..., 255`. This doubles
the population cost relative to the baseline (256 calls instead of 128), but
`syntax_property` is an O(1) char-table lookup and population happens once
per `re_compile_pattern` call.

## Comparison Table

| Property | Baseline (128) | This variant (256) |
|---|---|---|
| Struct size increase | 128 bytes | 256 bytes |
| Coverage | ASCII (0-127) | Full byte range (0-255) |
| Population calls | 128 | 256 |
| Benefit for unibyte | Partial | Complete |
| Benefit for multibyte | Complete for ASCII chars | Same (multibyte codes > 255) |
