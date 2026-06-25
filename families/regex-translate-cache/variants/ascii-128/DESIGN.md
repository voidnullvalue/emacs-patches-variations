# Design: Regex Translate Cache — ASCII-128
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline caches translations for byte codes 0–255 (256 entries, 256 bytes).
This variant reduces coverage to 0–127 (128 entries, 128 bytes). The saved
128 bytes per compiled pattern may matter in environments with many simultaneously
active patterns.

The rationale is that case-folded regex searches in Emacs are overwhelmingly
ASCII operations: `isearch`, `occur`, `grep-find`, and similar operations work
on ASCII identifiers and keywords. The 128-entry cache covers this common case
while deferring to `char_table_translate` for byte codes 128–255.

## Coverage Analysis

Codes not covered (128–255) include:
- Latin-1 characters in unibyte buffers (e.g., 'é', 'ñ', 'ü').
- Leading bytes of multibyte sequences (handled by `CHAR_TO_BYTE_SAFE` in the baseline).

For sessions that do case-folded search on Latin-1 text (e.g., `occur` in a
buffer with accented characters), this variant provides no improvement for those
characters. Whether this matters depends on workload.

## Comparison Table

| Property | Baseline (0-255) | ASCII-128 (0-127) |
|---|---|---|
| Coverage | Full byte range | ASCII only |
| Struct bytes | 256 | 128 |
| Compile calls | 256 char_table_translate | 128 char_table_translate |
| Latin-1 benefit | Yes | No (slow path) |
| Multibyte benefit | Partial (fast for ASCII chars) | Same |
