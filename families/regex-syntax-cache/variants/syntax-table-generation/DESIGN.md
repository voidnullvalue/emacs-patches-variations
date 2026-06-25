# Design: Regex Syntax Cache — Syntax-Table Generation Counter
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline relies on `re_compile_pattern` being called with a fresh pattern
whenever the syntax table changes — a guarantee made by the `compile_pattern`
wrapper in `search.c`. This is an implicit, non-local contract: any code path
that modifies the syntax table without triggering `compile_pattern` would silently
use stale syntax codes.

A generation counter makes the invalidation explicit and local. Every syntax-table
mutation increments `current_syntax_table_generation`. The cache is valid only
when `bufp->syntax_table_gen_built == current_syntax_table_generation`. A mismatch
at match time is detected in `SYNTAX_TRU` without any pattern recompilation.

## Generation Counter Design

```c
/* In syntax.c: */
uint32_t current_syntax_table_generation;

/* Increment at every syntax mutation site: */
void Fmodify_syntax_entry (...) { ...; current_syntax_table_generation++; }
void Fset_syntax_table (...) { ...; current_syntax_table_generation++; }
```

The counter is `uint32_t` (not `uint64_t`) to fit in the `re_pattern_buffer`
struct without excessive overhead. Overflow at 2^32 wraps to 0 — a false match
if the current generation also happens to be 0. In practice this cannot occur
in any normal session: 4 billion syntax-table mutations would require millions
of hours of editing. Document the theoretical overflow behavior.

## Mutation Sites to Instrument

The following mutation paths must increment the generation:
- `Fmodify_syntax_entry` — direct modification of a syntax table entry.
- `Fset_syntax_table` — installs a new syntax table for the current buffer.
- `Fcopy_syntax_table` — may create a mutable copy; the original is unchanged
  but the copy should be considered a new generation if it replaces a live table.
- Buffer-local inheritance changes (`Fset_char_table_parent` on a syntax table).

Paths that cannot be safely covered: runtime property changes via text properties
when `parse_sexp_lookup_properties` is set (the baseline disables the cache
entirely in that case via the `!parse_sexp_lookup_properties` guard, which
this variant preserves).

## Comparison Table

| Property | Baseline | Generation counter |
|---|---|---|
| Invalidation mechanism | Recompile pattern | Generation mismatch at match time |
| Explicit mutation tracking | No (relies on search.c contract) | Yes (mutation sites increment counter) |
| Stale cache detection | At next compile | At next match |
| Cross-buffer invalidation | Per-compile | Process-global (conservative) |
| Mutation-path audit required | No | Yes |
