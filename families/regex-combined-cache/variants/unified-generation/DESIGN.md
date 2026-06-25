# Design: Regex Combined Cache — Unified Generation Variant
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline combined cache uses two independent `bool_bf` flags
(`trt_ascii_valid`, `syntax_ascii_valid`), one per cache. Each flag must be
set independently and must be cleared independently when its respective table
changes. There is no single "all caches are stale" operation.

This variant replaces the two flags with a single `uint32_t cache_gen` field
per pattern and a single global `uint32_t re_global_cache_gen` counter. When
`re_invalidate_caches()` is called (from any syntax table or translate table
mutation site), `re_global_cache_gen` is incremented. On the next access to a
pattern's SYNTAX_TRU or TRANSLATE_TRU, the mismatch is detected and the slow
path is taken.

## Generation Counter Protocol

1. `re_global_cache_gen` starts at 1 (0 = invalid sentinel).
2. At `re_compile_pattern` time, if caches are built, set `bufp->cache_gen = re_global_cache_gen`.
3. At any syntax/translate mutation, call `re_invalidate_caches()` which increments
   `re_global_cache_gen`.
4. On the next match, `bufp->cache_gen != re_global_cache_gen` forces slow path.
5. The next call to `re_compile_pattern` rebuilds caches and restores `cache_gen`.

## Why a Single Counter for Both Caches

In practice, syntax table changes and translate table changes have independent
triggers. However, both tables are captured at compile time, so any mutation to
either requires recompiling the pattern (or at minimum rebuilding the cache).
A single invalidation covers both caches simultaneously: if either table changed,
both caches need rebuilding.

The per-cache ready bits (`syntax_ascii_ready`, `trt_ascii_ready`) handle the case
where only one cache is populated at compile time (e.g., no translate table present).
A generation match is necessary but not sufficient; the ready bit confirms the
specific cache was built.

## Comparison with Baseline

| Property | Baseline | This variant |
|---|---|---|
| Invalidation mechanism | Bool flag per cache | Global generation counter |
| Invalidation granularity | Per-cache | All caches simultaneously |
| Invalidation trigger | Recompile pattern | re_invalidate_caches() call |
| Extra struct bytes | 2 bool_bf bits | 4 bytes (uint32_t) + 2 bits |
| Integration points | search.c compile_pattern | All syntax/translate mutation sites |

## Wrap Safety

At 2^32 increments, `re_global_cache_gen` wraps to 0 (the invalid sentinel).
All existing patterns then have `cache_gen != 0` which... actually equals 0 on
the next recompile when `re_global_cache_gen == 0`. This is a correctness hazard:
add 1 check: if `re_global_cache_gen == 0` after increment, set it to 1.
