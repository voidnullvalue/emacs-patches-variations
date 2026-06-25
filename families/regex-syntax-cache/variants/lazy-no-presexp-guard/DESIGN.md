# Design: Regex Syntax Cache — Runtime Guard Variant
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline disables the syntax cache entirely when `parse_sexp_lookup_properties`
is non-nil. This is conservative: it ensures correctness (syntax codes may vary
by position) but abandons the optimization for all major modes that set this
variable (e.g., `cc-mode`, `web-mode`).

In practice, even when `parse_sexp_lookup_properties` is set, most character
positions do not have a `syntax-table` text property. Only the rare characters
with explicit syntax overrides need the full char-table walk.

## Approach

Remove the compile-time `!parse_sexp_lookup_properties` guard from the cache
initialization. Instead, move the per-position check into the `SYNTAX_TRU` macro:

```c
#define SYNTAX_TRU_POS(c, charpos)                               \
  ((bufp->syntax_ascii_valid && (unsigned)(c) < 128              \
    && !position_has_syntax_property(charpos))                   \
   ? (enum syntaxcode) bufp->syntax_ascii[(unsigned char)(c)]    \
   : SYNTAX(c))
```

`position_has_syntax_property(charpos)` calls `Fget_text_property` to check
whether charpos has a `syntax-table` property. If yes, fall back to `SYNTAX(c)`.
If no, use the cached value.

## Cost Analysis

For a buffer where 1% of characters have syntax-table text properties:
- 99% of SYNTAX_TRU calls: `bool check + bounds check + array load` (fast)
- 1% of SYNTAX_TRU calls: `bool check + bounds check + Fget_text_property + SYNTAX()` (slow path)

For a buffer where 0% of characters have text properties (the common case in
most major modes), all calls take the fast path, and the optimization is as
effective as the baseline.

For a buffer where 100% of characters have text properties (pathological), every
call takes the full slow path plus the extra Fget_text_property overhead — worse
than the baseline.

## Correctness

`position_has_syntax_property` queries the text property at `charpos`. The
caller must pass the correct current position. The `PTR_TO_OFFSET(d)` macro
converts the current string pointer `d` to a character position; this is correct
in the contexts where SYNTAX_TRU is called (inside `re_match_2_internal` after
PREFETCH, where `d` points to the current character in string1/string2).

## When to Prefer This Variant

Prefer this variant when:
- The target workload uses major modes that set `parse_sexp_lookup_properties`
- AND most characters lack explicit syntax-table text properties
- AND the performance delta justifies the additional macro complexity
