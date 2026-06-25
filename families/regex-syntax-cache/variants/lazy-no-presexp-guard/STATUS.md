# Status: Regex Syntax Cache — Runtime Guard Variant
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/regex-syntax-cache/variants/lazy-no-presexp-guard/implementation.patch
```

## Build

Not tested. The patch as written only shows the two SYNTAX_TRU call sites in the
word-boundary opcodes. All other SYNTAX_TRU call sites from the baseline must be
similarly updated to use `SYNTAX_TRU(c)` (which expands to `SYNTAX_TRU_POS(c,
PTR_TO_OFFSET(d))`).

## Known Issues

- `PTR_TO_OFFSET(d)` must be defined and correct at every SYNTAX_TRU call site.
  Verify that `d` is always the current string pointer when SYNTAX_TRU is called.
- `Fget_text_property` with charpos outside the buffer raises an error. Add a
  bounds check in `position_has_syntax_property` if needed.
- This variant is potentially slower than the baseline when `parse_sexp_lookup_properties`
  is set and many positions have text properties. Profile before adopting.

## Testing

See `tests/regex-syntax-cache/` for test cases covering text-property positions.
