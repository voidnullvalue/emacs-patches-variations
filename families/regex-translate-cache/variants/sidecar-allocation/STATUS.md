# Status: Regex Translate Cache — Sidecar Allocation
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/regex-translate-cache/variants/sidecar-allocation/implementation.patch
```

## Build

All platforms. Modifies regex-emacs.c and regex-emacs.h.

## Known Issues

- `table_ref` (a `Lisp_Object` in a malloc'd region) may not be visible to the
  GC mark phase. If the translate table is subject to collection, `table_ref`
  becomes a dangling pointer. In practice, translate tables passed to
  `re_compile_pattern` are kept alive by the caller, but this should be
  documented and audited.
- `translate_sidecar` allocation is skipped when `NILP(translate)` — but the
  sidecar pointer remains NULL. The TRANSLATE_TRU macro checks for NULL, so no
  crash; but a pattern that gains a translate table on recompile must allocate
  the sidecar at that point.
- Locate all error exit paths (goto, return) in `re_compile_pattern` to ensure
  `xfree(bufp->translate_sidecar); bufp->translate_sidecar = NULL` is called.

## Testing

- After compile with non-nil translate: sidecar != NULL, sidecar->valid == true.
- After compile with nil translate: sidecar->valid == false (sidecar may or may not be NULL).
- After regfree: sidecar == NULL.
- Double regfree: no crash (second call checks NULL before xfree).
