# Design: Regex Translate Cache — Identity Sentinel
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline stores translations as `unsigned char`, which cannot distinguish
"translation is 0 (NUL → NUL, identity)" from "translation is 0 (NUL → NUL,
non-identity that happens to equal 0)." In practice, NUL is its own translation
in any reasonable case-fold table, but the ambiguity is architectural.

A sentinel value (`0xFFFF`) in a `uint16_t` entry explicitly encodes "identity
translation" without needing a separate bit. This removes the ambiguity and
makes the representation self-documenting.

## Sentinel Value Choice

`0xFFFF` is:
- Not a valid Unicode code point (it is a "non-character" in Unicode).
- Not a valid byte translation result (byte results are 0–255 = 0x00–0xFF).
- Representable in `uint16_t` without overflow.

Therefore, `0xFFFF` can safely be used as a sentinel meaning "return C unchanged."

## Population Logic

```c
for (int i = 0; i < 128; i++) {
    int t = !NILP(translate) ? char_table_translate(translate, i) : i;
    if (t == i)
        trt_ascii[i] = 0xFFFF;       // identity
    else if (t >= 0 && t <= 254)
        trt_ascii[i] = (uint16_t) t; // non-identity in byte range
    else
        trt_ascii[i] = 0xFFFF;       // out of range: sentinel = safe slow path
}
```

Out-of-range results (> 254) use the sentinel, causing `TRANSLATE_TRU` to fall
back to `TRANSLATE(C)`. This is conservative but correct.

## Memory

128 × `uint16_t` = 256 bytes — same as the baseline's 256 × `unsigned char`.
The memory cost is the same for 128-entry coverage but uses wider entries.

## Comparison Table

| Property | Baseline (unsigned char) | Identity sentinel (uint16_t) |
|---|---|---|
| Entry size | 1 byte | 2 bytes |
| Coverage | 0-127 fast path | 0-127 fast path |
| Identity encoding | Stored as value | Explicit 0xFFFF |
| NUL ambiguity | Present | Resolved |
| Memory for 128 entries | 128 bytes | 256 bytes |
