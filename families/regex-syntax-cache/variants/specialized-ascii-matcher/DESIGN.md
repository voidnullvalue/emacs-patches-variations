# Design: Regex Syntax Cache — Specialized ASCII Matcher
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline inserts `SYNTAX_TRU(c)` at individual call sites within
`re_match_2_internal` — a function of ~2000 lines. This approach works but
requires touching the general matcher's opcode dispatch loop, which introduces
risk of subtle breakage in non-ASCII paths.

This variant takes a different integration point: dispatch before entering
the matcher. A thin wrapper `re_match_2_select` checks whether the pattern
qualifies for a specialized ASCII matching path. If it does, the specialized
path handles word-boundary opcodes with direct flat-array syntax lookups. If
not (multibyte pattern, no syntax cache, parse_sexp_lookup_properties set),
control falls through to the unmodified `re_match_2_internal`.

## Dispatch Logic

```c
ptrdiff_t re_match_2_select(...) {
  if (!RE_MULTIBYTE_P(bufp)
      && bufp->syntax_ascii_valid
      && !parse_sexp_lookup_properties)
    return re_match_ascii_fast(bufp, ..., bufp->syntax_ascii);
  return re_match_2_internal(bufp, ...);
}
```

## Preconditions

- `!RE_MULTIBYTE_P(bufp)`: pattern compiled for unibyte input.
- `syntax_ascii_valid`: cache is populated and valid.
- `!parse_sexp_lookup_properties`: text-property lookups are not needed.

When all hold, `re_match_ascii_fast` handles the pattern. It scans the opcode
stream for word-boundary opcodes and uses `syntax_cache[c]` (direct array
access, no macro overhead). For any opcode it does not handle, it falls back
to `re_match_2_internal`.

## Differential Testing

The `re_match_2_select` wrapper enables differential testing: both paths are
callable on the same pattern with the same input. Tests can compare results
from `re_match_ascii_fast` and `re_match_2_internal` on identical inputs to
verify the specialized path produces identical matches.

## Comparison Table

| Property | Baseline (per-site macros) | Specialized matcher (pre-dispatch) |
|---|---|---|
| Integration point | Within general matcher opcode loop | Before matcher entry |
| General matcher modified | Yes | No |
| Differential testing | Hard | Natural |
| Non-ASCII path risk | Slight (macro at call sites) | None (general matcher unchanged) |
| Dispatch overhead | None (inline macro) | One function call per match |
