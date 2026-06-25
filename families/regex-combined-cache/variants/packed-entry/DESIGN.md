# Design: Combined Regex Cache — Packed Entry
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline combined cache uses 128 + 256 = 384 bytes for the two separate arrays
plus two validity bits. The per-character struct uses 512 bytes. This variant
achieves the minimum possible storage: 256 bytes (128 × uint16_t) for both caches
combined, by packing syntax and translate data into one 16-bit scalar per character.

The tradeoff is complexity: field access requires masks and shifts instead of
simple array dereferences.

## Bit Layout

```
bit 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
     R  T  --------t--------  S  ----s----
     │  │           │         │       │
     │  │           │         │       └── syntax code (5 bits, 0-31)
     │  │           │         └────────── syntax_valid (1 bit)
     │  │           └──────────────────── translate value (8 bits, 0-255)
     │  └──────────────────────────────── translate_valid (1 bit)
     └─────────────────────────────────── reserved (0)
```

Total: 16 bits = `uint16_t`.

## Field-Width Sufficiency

- **Syntax (5 bits):** `enum syntaxcode` in Emacs has values 0–`Smax`. As of
  Emacs 29, `Smax <= 14`. Five bits (0–31) provides margin for future syntax codes.
  Add `_Static_assert(Smax <= RE_PACK_SYNTAX_MASK, "syntax code exceeds 5 bits")`.
- **Translate (8 bits):** Byte values 0–255. Translations resulting in code points
  > 255 (e.g., Unicode case mappings) are not cached; the valid bit is left clear.

## Overflow Safety

Values that do not fit their fields leave the corresponding valid bit clear. The
macros test `valid` before extracting the field, so uncached values fall back to
the slow path. No silent truncation occurs.

## Comparison Table

| Property | Baseline SoA | Per-char struct AoS | Packed uint16_t |
|---|---|---|---|
| Memory | 384+ bytes | 512 bytes | 256 bytes |
| Access mechanism | Array dereference | Struct field | Mask + shift |
| Field-width limit | 8-bit syntax (truncated) | 8-bit each | 5-bit syntax, 8-bit translate |
| Code complexity | Low | Low | Medium |
| Vectorizable | Yes (SoA) | No | Maybe (SIMD on packed) |
