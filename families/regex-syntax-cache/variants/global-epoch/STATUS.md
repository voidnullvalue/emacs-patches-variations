# Status: Regex Syntax Cache — Global Epoch Variant
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/regex-syntax-cache/variants/global-epoch/implementation.patch
```

## Build

Requires:
1. `emacs_syntax_table_epoch` declared as `extern uintmax_t` in `src/syntax.h`
2. Epoch incremented at all syntax table mutation sites in `src/syntax.c`

The patch as provided adds the definition in `syntax.c` but only as a stub.
A complete implementation requires locating all mutation sites.

## Known Issues

- The patch's `syntax.c` section shows only the variable definition, not the
  increment sites. Without bumping the epoch at every mutation, the cache can
  become stale. This is the primary implementation risk.
- `emacs_syntax_table_epoch` wraps at `UINTMAX_MAX`. On wrap, epoch 0 is
  `GLOBAL_SYNTAX_CACHE_EPOCH_INVALID`, so the next `re_compile_pattern` will
  rebuild the cache (correct behavior, but one extra rebuild).
- Global state limits future parallelization. Note this in any future
  multi-threaded Emacs work.
