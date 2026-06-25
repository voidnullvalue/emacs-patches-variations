# Status: Combined Regex Cache — Independently Lazy
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/regex-combined-cache/variants/independently-lazy/implementation.patch
```

## Build

All platforms. Modifies regex-emacs.c and regex-emacs.h.

## Known Issues

- `translate_ref` (Lisp_Object in re_pattern_buffer) may not be seen by GC.
  Same caveat as regex-translate-cache/table-generation. Document and audit.
- The comma operator in `SYNTAX_TRU(c) = (ensure_syntax_cache(bufp), ...)` is
  valid C but unusual. Some static analyzers may flag it. An alternative is to
  wrap the macro call in a helper function.
- `ensure_syntax_cache` reads `parse_sexp_lookup_properties` which is a
  buffer-local variable. Ensure it is set correctly before the first
  `SYNTAX_TRU` call in any match invocation.
- After `re_compile_pattern`, both `built` flags are false and both `valid`
  flags are false. They are set on first use. A recompile must reset both
  `built` flags to force re-population.

## Testing

- Pattern that never reaches word-boundary opcode: `syntax_ascii_built == false`
  after matching.
- Pattern with word-boundary: after first match, `syntax_ascii_built == true`.
- Pattern without translate table: `trt_ascii_built == false` after matching.
- Partial invalidation: reset only `trt_ascii_built`; verify syntax path still
  uses the fast path without rebuilding.
