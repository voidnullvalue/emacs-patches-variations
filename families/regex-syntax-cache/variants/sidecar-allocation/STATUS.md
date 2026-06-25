# Status: Regex Syntax Cache — Sidecar Allocation
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/regex-syntax-cache/variants/sidecar-allocation/implementation.patch
```

## Build

All platforms. Requires that `xmalloc` and `xfree` are available (standard
in Emacs alloc.c; present in regex-emacs.c via emacs.h includes).

## Known Issues

- `xmalloc` in Emacs aborts on allocation failure rather than returning NULL;
  the NULL-check in `SYNTAX_TRU` protects against `syntax_sidecar` being NULL
  before the first compilation, not against allocation failure.
- Locating all error exit paths in `re_compile_pattern` for `xfree` placement
  requires careful audit; missing one causes a memory leak on compilation failure.
- If `re_compile_pattern` is reentrant or called from multiple threads
  simultaneously on the same `bufp`, the `NULL`-check before allocation is
  a race condition. Emacs's current design avoids this, but document the
  assumption.

## Testing

- After compile: `bufp->syntax_sidecar != NULL`, `syntax_sidecar->valid == true`.
- After `regfree`: `bufp->syntax_sidecar == NULL`.
- Double-free test: call `regfree` twice; second call should not crash.
- Compilation failure: trigger an error mid-compile; verify `syntax_sidecar` is freed.
