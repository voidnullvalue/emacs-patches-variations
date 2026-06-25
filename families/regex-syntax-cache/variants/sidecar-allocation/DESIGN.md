# Design: Regex Syntax Cache — Sidecar Allocation
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline embeds a 128-byte `syntax_ascii` array directly in
`re_pattern_buffer`. Every compiled pattern pays the 128-byte struct overhead
even if the pattern never uses a word-boundary opcode that would benefit from
the cache. For workloads with many compiled patterns (a large buffer-local
syntax table or high compilation rate), this adds memory pressure proportional
to the pattern count.

Moving the cache to a separately allocated sidecar reduces the per-pattern
struct overhead to one pointer (8 bytes on LP64) and defers the allocation until
a pattern actually needs the cache.

## Sidecar Structure

```c
struct re_syntax_sidecar {
  unsigned char syntax[128];  /* syntax codes for ASCII 0-127 */
  bool          valid;        /* true when syntax[] is populated */
};
```

The `re_pattern_buffer` field changes from:
```c
unsigned char syntax_ascii[128];
bool_bf       syntax_ascii_valid : 1;
```
to:
```c
struct re_syntax_sidecar *syntax_sidecar;  /* NULL until allocated */
```

## Lifecycle

1. **Allocation:** `xmalloc(sizeof(struct re_syntax_sidecar))` in `re_compile_pattern`,
   only if `syntax_sidecar == NULL` (reuse across recompilations of the same buffer).
2. **Population:** Set `valid = false` first (reset on reuse), then populate
   `syntax[0..127]` if `!parse_sexp_lookup_properties`.
3. **Free:** `xfree(bufp->syntax_sidecar); bufp->syntax_sidecar = NULL` in:
   - `regfree()` (normal deallocation path).
   - All error exit paths of `re_compile_pattern` (compilation failure cleanup).
4. **NULL-check in macro:** `SYNTAX_TRU` checks `bufp->syntax_sidecar &&
   bufp->syntax_sidecar->valid` before array access.

## Double-Free Prevention

Setting `bufp->syntax_sidecar = NULL` immediately after `xfree` prevents
double-free if regfree is called more than once (it becomes a no-op on the
second call since NULL is not freed).

## Comparison Table

| Property | Baseline (inline) | Sidecar allocation |
|---|---|---|
| Struct overhead per pattern | 128 bytes | 8 bytes (pointer) |
| Sidecar allocation | None | 1 per pattern |
| Hot path extra dereference | No | Yes (pointer + field) |
| Error-path cleanup | None | xfree in error paths |
| Patterns without word-boundary | Pay 128 bytes | Pay 8 bytes |
