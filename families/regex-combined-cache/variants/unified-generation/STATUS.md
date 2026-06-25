# Status: Regex Combined Cache — Unified Generation Variant
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/regex-combined-cache/variants/unified-generation/implementation.patch
```

## Build

Requires `<stdint.h>` for `uint32_t`. The `extern` declarations in the `struct`
definition are incorrect C — move them to a header before the struct definition.

## Known Issues

- `extern` declarations inside a struct body is not valid C. The patch as written
  shows the intent but must be restructured: put `extern uint32_t re_global_cache_gen;`
  in `regex-emacs.h` before the struct, not inside it.
- Generation wrap at 0 is not handled. Add: after `re_global_cache_gen++`, if the
  result is 0, set it to 1.
- The partial population case (only translate or only syntax) needs careful handling
  of the ready bits. The current patch has a comment but does not fully implement
  the ready bit checks in SYNTAX_TRU and TRANSLATE_TRU.
- All syntax table mutation functions in `syntax.c` and `buffer.c` must call
  `re_invalidate_caches()`. This is a significant integration burden; audit carefully.
