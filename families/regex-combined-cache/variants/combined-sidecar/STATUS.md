# Status: Combined Regex Cache — Combined Sidecar
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/regex-combined-cache/variants/combined-sidecar/implementation.patch
```

## Build

All platforms. Modifies regex-emacs.c and regex-emacs.h.

## Known Issues

- `xmalloc` in Emacs aborts on allocation failure. The NULL-check in macros
  is a compile-before-match guard, not an allocation-failure guard.
- Sidecar `translate[]` is 256 bytes even though the translate fast path in
  this variant covers only codes < 128. The extra 128 bytes are reserved for
  future extension or for matching the baseline's full-byte coverage.
- Locate all `goto fail`, early `return`, and exception-path `RETVAL` exits
  in `re_compile_pattern` to ensure `xfree(bufp->combined_cache)` is placed
  correctly. Missing one causes a memory leak on compilation failure.
- On regex buffer reuse (same `bufp` passed to two consecutive
  `re_compile_pattern` calls), the existing sidecar is reused — but only if
  `combined_cache != NULL`. If the first compile set it to NULL (error path),
  the second compile allocates a new one.

## Testing

- After compile: `combined_cache != NULL`.
- After regfree: `combined_cache == NULL`.
- Error path test: trigger compilation failure; verify `combined_cache == NULL`.
- Double regfree: no crash.
- Reuse test: compile twice on same bufp; verify only one allocation.
