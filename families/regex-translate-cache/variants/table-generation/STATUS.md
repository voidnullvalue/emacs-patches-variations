# Status: Regex Translate Cache — Table-Generation (EQ Identity)
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/regex-translate-cache/variants/table-generation/implementation.patch
```

## Build

All platforms. Adds a `Lisp_Object` field to `re_pattern_buffer`; ensure the
struct is correctly aligned (Lisp_Object is pointer-sized on all platforms).

## Known Issues

- `trt_built_for` is not scanned by the GC (re_pattern_buffer is malloc'd).
  If the translate table can be garbage-collected while the pattern is live,
  `trt_built_for` becomes a dangling pointer. Document and audit.
- `EQ(bufp->trt_built_for, translate)` is evaluated on every `TRANSLATE_TRU`
  call. On platforms where `EQ` is not inlined, this may add measurable overhead
  for regex-heavy workloads. Benchmark if concerned.
- The `trt_built_for = Qnil` initialization must happen before the first
  `TRANSLATE_TRU` call on a freshly compiled pattern.

## Testing

- Compile with table A; store trt_built_for = A; match → fast path.
- Switch to table B (without recompile); trt_built_for ≠ B → slow path.
- After recompile with B; trt_built_for = B → fast path again.
- Compile with nil translate; trt_built_for = Qnil; match with nil translate → EQ → no fast path needed (trt_ascii_valid = false anyway).
