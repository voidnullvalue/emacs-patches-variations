# Design: Combined Regex Cache — Independently Lazy
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline combined cache populates both subcaches eagerly at `re_compile_pattern`
time. This pays the full cost (128 + 256 `char_table` lookups) for every pattern,
even if:
- The pattern never uses a word-boundary opcode (syntax cache never queried).
- The pattern is compiled without a translate table (translate cache never used).

This variant defers population to the first use of each subcache, using a pair of
`built` flags to avoid re-entering the populate loop on subsequent misses.

## Flag Design

Two pairs of flags:
- `syntax_ascii_built` / `syntax_ascii_valid`
- `trt_ascii_built` / `trt_ascii_valid`

The `built` flag means "the ensure function has run at least once." The `valid`
flag means "the entries are populated and usable." The two can disagree when
`parse_sexp_lookup_properties` is true (built=true, valid=false).

## Population Functions

```c
static void ensure_syntax_cache(struct re_pattern_buffer *bufp) {
  if (bufp->syntax_ascii_built) return;   // fast early exit after first call
  bufp->syntax_ascii_built = true;
  if (parse_sexp_lookup_properties) return;  // cannot cache
  for (int i = 0; i < 128; i++)
    bufp->syntax_ascii[i] = (unsigned char) syntax_property(i, 0);
  bufp->syntax_ascii_valid = true;
}
```

`SYNTAX_TRU` calls `ensure_syntax_cache(bufp)` before the cache check. On
the first call, `built` is false and `ensure` runs. On all subsequent calls,
`built` is true and `ensure` returns immediately (one branch, branch-predicted).

## Translate Reference

The translate table (a `Lisp_Object`) is stored in `bufp->translate_ref` at
compile time so `ensure_translate_cache` can populate the array without access
to the `translate` local variable in `re_compile_pattern`.

## Comparison Table

| Property | Baseline (eager) | Independently lazy |
|---|---|---|
| Syntax population | At compile | At first SYNTAX_TRU |
| Translate population | At compile | At first TRANSLATE_TRU |
| Extra flags | 0 | 4 (2 built + 2 valid) |
| Hot path overhead | None (valid check only) | One function call (branch-predicted) |
| Patterns without word-boundary | Pay syntax population | Skip it |
