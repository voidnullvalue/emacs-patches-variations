# Status: Regex Translate Cache — Lazy Bitmap Variant
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/regex-translate-cache/variants/lazy-bitmap/implementation.patch
```

## Build

Not tested. Requires `<string.h>` for `memset` (already included in regex-emacs.c).

## Known Issues

- `TRT_BIT_FILL` uses a do-while macro with side effects (sets a bit, writes to
  trt_ascii). This is safe in the current single-threaded context but must not
  be called from multiple threads without a lock.
- The `TRANSLATE_TRU` macro calls `TRT_BIT_FILL` inside a comma-expression
  `(TRT_BIT_FILL(bufp, C), bufp->trt_ascii[...])`. The comma operator sequences
  the side effect before the load. This is valid C but the macro is complex;
  verify with a preprocessor expansion check.
- `trt_bitmap_valid` uses the same name as the baseline's `trt_ascii_valid`.
  These are different fields with different semantics; do not apply both patches
  simultaneously.

## Testing

See `tests/regex-translate-cache/` for correctness tests against known translation tables.
