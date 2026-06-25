# Design: Combined Regex Cache — Per-Character Struct
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline combined cache uses two separate arrays (Structure of Arrays, SoA):
`syntax_ascii[128]` and `trt_ascii[256]`, each with its own validity flag. The
two subcaches are independently valid but the validity flags are coarse-grained
(one flag per subcache, not per character).

This variant uses an Array of Structs (AoS): one `ReCharEntry` per ASCII code,
with `syntax_valid` and `translate_valid` bits per character. This allows:
- The syntax entry for character 65 ('A') to be valid while the translate entry
  for the same character is not yet populated.
- Selective invalidation: if only the translate table changes, the syntax entries
  remain valid without any struct clear.

## Structure

```c
typedef struct {
  unsigned char syntax;
  unsigned char translate;
  bool          syntax_valid;
  bool          translate_valid;
} ReCharEntry;  // 4 bytes on typical LP64 (2 chars + 2 bools, padded)

ReCharEntry re_char_cache[128];   // 512 bytes total
bool_bf     re_char_cache_valid : 1;
```

## Initialization

`memset(re_char_cache, 0, sizeof(re_char_cache))` followed by:
- Syntax population: iterate 0..127, set `syntax` and `syntax_valid = true`.
- Translate population: iterate 0..127, set `translate` and `translate_valid = true`.

The `re_char_cache_valid` outer flag short-circuits both macro guards when false.

## AoS vs SoA Tradeoff

AoS (`re_char_cache[i].syntax`, `re_char_cache[i].translate`) places both values
for character `i` adjacent in memory. This helps when both subcaches are accessed
for the same character within one opcode evaluation. SoA (two separate arrays)
benefits from vectorized access across multiple characters, which is less relevant
in the matcher's opcode dispatch model.

## Comparison Table

| Property | Baseline SoA | Per-character AoS |
|---|---|---|
| Memory | 384+ bytes | 512 bytes |
| Validity granularity | Per-subcache | Per-character + per-subcache |
| Partial invalidation | Coarse (whole subcache) | Fine (per-character) |
| Cache layout | SoA | AoS |
| Struct padding | None (flat arrays) | ~4 bytes per entry |
