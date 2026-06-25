# Design: Combined Regex Cache — Combined Sidecar
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline combined cache embeds both the 128-byte syntax array and the 256-byte
translate array directly in `re_pattern_buffer`, adding ~384 bytes to every
compiled pattern. This variant moves both to a single sidecar allocation, reducing
the struct overhead to one pointer (8 bytes) at the cost of one `xmalloc` per
compiled pattern.

This is a cross-family composition: it applies the sidecar allocation strategy
(explored separately in `regex-syntax-cache/variants/sidecar-allocation` and
`regex-translate-cache/variants/sidecar-allocation`) to the combined cache, with
a single allocation covering both subcaches.

## Sidecar Structure

```c
struct re_combined_sidecar {
  unsigned char syntax[128];      /* syntax codes for ASCII 0-127 */
  unsigned char translate[256];   /* translated values for byte codes 0-255 */
  bool          syntax_valid;
  bool          translate_valid;
};  // ~386 bytes
```

One `xmalloc(sizeof(struct re_combined_sidecar))` covers both subcaches. The
`re_pattern_buffer` field changes from 384+ bytes of inline storage to a single
8-byte pointer.

## Lifecycle

1. **Allocation:** `xmalloc(sizeof(struct re_combined_sidecar))` in `re_compile_pattern`
   if `combined_cache == NULL`.
2. **Reuse:** If `combined_cache != NULL` (recompile of existing buffer), reset
   both `*_valid` flags and re-populate.
3. **Partial initialization:** Set `syntax_valid` only if syntax was populated;
   set `translate_valid` only if translate was populated.
4. **Free:** `xfree(bufp->combined_cache); bufp->combined_cache = NULL` in
   `regfree()` and all error exit paths.

## NULL-Check in Macros

`SYNTAX_TRU` and `TRANSLATE_TRU` both check `bufp->combined_cache != NULL` before
accessing fields. This guards against calling the macro before any compilation
(combined_cache is uninitialized or zeroed to NULL).

## Comparison Table

| Property | Baseline (inline) | Combined sidecar |
|---|---|---|
| re_pattern_buffer overhead | ~384 bytes | 8 bytes (pointer) |
| Sidecar allocation | None | 1 xmalloc |
| Allocations for both subcaches | N/A | 1 |
| vs separate sidecars | N/A | 1 alloc instead of 2 |
| Error-path cleanup | None | xfree in error paths |
| Null-check in macros | No | Yes |
