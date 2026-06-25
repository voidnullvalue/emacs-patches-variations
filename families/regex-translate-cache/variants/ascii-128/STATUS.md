# Status: Regex Translate Cache — ASCII-128
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/regex-translate-cache/variants/ascii-128/implementation.patch
```

## Build

All platforms. Single-file struct change in regex-emacs.h plus macro and loop change.

## Known Issues

- The `(int) bufp->trt_ascii[(unsigned char)(C)]` cast preserves the translation
  value as a non-negative int. Callers that previously received an `unsigned char`
  may see signed promotion behavior; ensure all call sites handle the return as
  an `int` (same as `TRANSLATE(C)` returns).
- Codes 128–255 take the slow path even in performance-sensitive case-folded
  search loops. Profile against the baseline on Latin-1 workloads to quantify
  the difference.

## Testing

- `trt_ascii[65]` should contain the lowercase of 'A' (65 → 97 for default case-fold).
- `trt_ascii[127]` should contain the identity (DEL has no case-fold).
- Code 128 should go through `TRANSLATE(C)` (slow path) and return the correct value.
- See `tests/regex-translate-cache/` for shared test infrastructure.
