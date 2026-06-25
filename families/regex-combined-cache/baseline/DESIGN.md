# Design: Regex Combined Cache Baseline
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

Applying `regex-syntax-cache` and `regex-translate-cache` as separate patches
fails because both add fields to `re_pattern_buffer` at the same location and
both add initialization blocks to the end of `re_compile_pattern`. The patches
conflict at the context level.

This combined patch resolves the conflict by merging both changes into a single
coherent diff, with correct ordering of struct fields and initialization blocks.

## Contents

Combines, in order:
1. `SYNTAX_TRU` macro definition (regex-emacs.c line ~50)
2. `TRANSLATE_TRU` macro definition (regex-emacs.c line ~1196)
3. All `TRANSLATE_TRU` usage sites (fastmap scan, match opcodes)
4. All `SYNTAX_TRU` usage sites (word-boundary and syntax-class opcodes)
5. Both initialization blocks at end of `re_compile_pattern`
6. Both struct fields in `re_pattern_buffer` (regex-emacs.h)

## Ordering

The `trt_ascii` initialization block precedes the `syntax_ascii` initialization
block. This is arbitrary; either order is correct because the two caches are
independent and neither affects the other's initialization.

## Independence

The two `bool_bf` flags `trt_ascii_valid` and `syntax_ascii_valid` are set
independently. It is possible for one to be true while the other is false (e.g.,
in a search without a translate table, `trt_ascii_valid` is false while
`syntax_ascii_valid` may be true). Both macros (`TRANSLATE_TRU`, `SYNTAX_TRU`)
fall back to the slow path when their respective flag is false.

## Alternative: Unified Generation Counter

See `variants/unified-generation` for an alternative that uses a single
generation counter to invalidate both caches simultaneously, reducing the struct
size and simplifying the invalidation logic.
