# Design: Regex Combined Cache — Search-Path Hook Variant
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

Both the baseline and the unified-generation variant populate the ASCII caches
at `re_compile_pattern` time. In Emacs, `compile_pattern` in `search.c` wraps
`re_compile_pattern` and is called from `search_buffer`, `Fre_search_forward`,
etc. Many search operations repeat the same pattern in a tight loop (e.g.,
`isearch`, `query-replace`, `font-lock-search`).

Each call to `compile_pattern` currently rechecks the pattern and potentially
recompiles. Even if recompile is skipped (pattern cached), the ASCII cache
population in `re_compile_pattern` runs again.

This variant adds a single-entry key-based cache at the `search.c` level that
short-circuits the entire `re_compile_pattern` call when the key matches.

## Key Structure

```c
typedef struct {
  unsigned char  *pattern_bytes;  /* copy of last pattern */
  ptrdiff_t       pattern_len;
  Lisp_Object     syntax_table;   /* pointer identity */
  Lisp_Object     translate;      /* pointer identity */
  struct re_pattern_buffer *bufp; /* last compiled pattern */
} SearchPathCache;
```

The key is (pattern text, syntax table, translate table). If all three match
the previous call, `sp_cache.bufp` is returned directly without any regex
compilation or cache population.

## Correctness

**Pattern text:** Compared byte-for-byte. Semantically identical patterns with
different byte representations (e.g., different whitespace normalization) will
be treated as different keys. This is conservative (may miss cache opportunities)
but never wrong.

**Syntax table identity:** `EQ(sp_cache.syntax_table, BVAR(current_buffer, syntax_table))`.
This uses Lisp_Object pointer equality. If a buffer changes its syntax table via
`set-syntax-table`, a new Lisp object is created and the pointer changes — correct.
If `modify-syntax-entry` modifies the existing syntax table object in-place, the
pointer does not change and the cache gives a stale result — **correctness bug**.
Fix: bump a global epoch counter in `modify-syntax-entry` and include it in the key.

**Translate table:** Same identity model. Same in-place-modification caveat.

## Single-Entry Limitation

The cache holds exactly one entry. If the calling code alternates between two
patterns (common in font-lock with alternating keyword patterns), the cache
will miss on every call. An LRU cache of N entries would help but adds complexity.

In practice, `isearch` and `query-replace` use a single pattern per interactive
session, so the single-entry cache covers the most common cases.

## Layering

This variant layers on top of the baseline regex-emacs.c patch. The per-pattern
ASCII caches are still populated on the first compile. On subsequent calls with
the same key, the pre-populated caches are reused without any overhead.
