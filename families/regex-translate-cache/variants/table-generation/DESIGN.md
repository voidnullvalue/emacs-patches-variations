# Design: Regex Translate Cache — Table-Generation (EQ Identity)
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline uses a `bool_bf trt_ascii_valid` flag. Once set at compile time,
it remains true until the pattern is recompiled. If the user replaces the
translate table (e.g., disables case folding and re-enables it with a different
table), the old cache values are used until the next `re_compile_pattern` call.
This is currently safe because `compile_pattern` in search.c forces recompile
on table change, but it is an implicit contract.

This variant makes the contract explicit in the macro: the cache is valid only
if the current `translate` is `EQ` to the table used when the cache was built.
Table replacement is detected at match time without any external signal.

## Validation

```c
#define TRANSLATE_TRU(C)  \
  ((bufp->trt_ascii_valid  \
    && EQ(bufp->trt_built_for, translate)  \
    && (unsigned)(C) < 128)  \
   ? trt_ascii[C] : TRANSLATE(C))
```

`EQ` is a pointer-identity comparison (Lisp_Object equality without type
checking). It detects replacement of the translate table with a different object.
It does **not** detect in-place mutation of the existing table's char-table
entries — that would require a generation counter on `Lisp_Object` char-tables,
which is a separate and larger change.

## Known Limitation: In-Place Mutation

If user code modifies the translate table after compilation:
```elisp
(aset case-fold-search-table ?A ?A)  ;; hypothetical in-place mutation
```
The cache is not invalidated, and the modified translation is not used until the
next recompile. This is documented as a known limitation; in practice Emacs does
not expose direct in-place mutation of the standard case-fold table.

## trt_built_for GC Safety

`trt_built_for` is a `Lisp_Object` stored in `re_pattern_buffer`, which is
malloc'd. If `re_pattern_buffer` is not visible to the GC mark phase, the
stored table may be collected if no other reference holds it. In practice,
translate tables are always referenced from global variables or active stack
frames during matching. Document this assumption in the patch.

## Comparison Table

| Property | Baseline (bool flag) | Table-generation (EQ) |
|---|---|---|
| Table replacement detection | No (relies on external recompile) | Yes (EQ mismatch) |
| In-place mutation detection | No | No |
| Hot path cost | 1 bool test | 1 bool + 1 EQ comparison |
| Struct addition | None | 1 Lisp_Object (8 bytes) |
