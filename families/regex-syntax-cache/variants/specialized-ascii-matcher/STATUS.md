# Status: Regex Syntax Cache — Specialized ASCII Matcher
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/regex-syntax-cache/variants/specialized-ascii-matcher/implementation.patch
```

## Build

Single-file change to `regex-emacs.c` (plus the unchanged struct field in
`regex-emacs.h` for `syntax_ascii` and `syntax_ascii_valid`).

## Known Issues

- The opcode scan in `re_match_ascii_fast` (checking for non-qualifying opcodes)
  is a simplified placeholder in this design-space patch; a production
  implementation must correctly track variable-length operand widths in the
  opcode stream.
- The fast path in this patch delegates back to `re_match_2_internal` after
  the feasibility scan — it is a dispatch optimization, not a fully independent
  matching path. A fully specialized path would require reimplementing the
  word-boundary opcode handlers, which would duplicate ~50 lines of matcher logic.
- `re_match_2_select` must replace all call sites of `re_match_2_internal`
  in the external API. The patch shows one representative site.

## Testing

Differential test: call both `re_match_ascii_fast` and `re_match_2_internal`
on the same unibyte pattern + ASCII string. Compare results.
Test mixed input: ASCII pattern on multibyte string → precondition fails →
dispatch to `re_match_2_internal`.
