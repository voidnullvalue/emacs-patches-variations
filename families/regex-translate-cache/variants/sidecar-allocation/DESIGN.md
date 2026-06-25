# Design: Regex Translate Cache — Sidecar Allocation
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline embeds a 256-byte `trt_ascii` array in `re_pattern_buffer`. For
patterns that are compiled without a translation table (`NILP(translate)`),
these 256 bytes are allocated but never used.

Moving the cache to a separately allocated sidecar reduces the per-pattern
struct overhead to one pointer (8 bytes) and allocates the 256-byte sidecar
only when `translate` is non-nil.

This mirrors the sidecar approach for the syntax cache but for the translate
family. The key difference is that the sidecar stores a `Lisp_Object table_ref`
field, enabling future stale-detection if the translate table is replaced.

## Sidecar Structure

```c
struct re_translate_sidecar {
  unsigned char trt[256];   /* translations for byte codes 0-255 */
  bool          valid;      /* true when trt[] is populated */
  Lisp_Object   table_ref;  /* translate table at build time */
};
```

The `table_ref` is an identity anchor: if `translate` is replaced by a
different table object, `table_ref != translate` can be detected. This is an
optional enhancement; the `valid` flag alone suffices for basic operation.

## GC Safety Note

`table_ref` is a `Lisp_Object` stored in a malloc'd region outside the Lisp
heap. If the GC does not scan this region, `table_ref` may become a dangling
reference if the translate table is garbage-collected. In Emacs, `translate`
is typically kept alive by being bound to a local variable or a global;
document this assumption and add the sidecar to an appropriate GC-visible
list if required.

## Comparison Table

| Property | Baseline (inline) | Sidecar allocation |
|---|---|---|
| Struct overhead | 256 bytes | 8 bytes (pointer) |
| Allocation | None | 1 xmalloc per pattern |
| Stale-detection support | No | Yes (table_ref) |
| Patterns without translate | 256 bytes wasted | 8 bytes (no alloc) |
| Hot path dereferences | 1 (array) | 2 (pointer + array) |
