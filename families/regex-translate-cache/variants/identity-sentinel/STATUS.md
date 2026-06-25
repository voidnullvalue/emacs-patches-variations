# Status: Regex Translate Cache — Identity Sentinel
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/regex-translate-cache/variants/identity-sentinel/implementation.patch
```

## Build

All platforms. Requires `<stdint.h>` for `uint16_t` (included by regex-emacs.c).

## Known Issues

- `TRT_IDENTITY_SENTINEL = 0xFFFF` should have a static assertion verifying that
  no valid byte translation result equals 0xFFFF. Add:
  `_Static_assert(TRT_IDENTITY_SENTINEL > 255, "sentinel conflicts with byte range");`
- Out-of-range translations (> 254) are silently mapped to the sentinel (slow path).
  Add a comment noting this and test that the slow path returns the correct result.
- The macro returns `(int)(C)` for sentinel entries and `(int) trt_ascii[C]` for
  non-sentinel entries. Ensure all call sites handle the `int` return type correctly.

## Testing

- Code 0 (NUL) with identity translation: `trt_ascii[0]` should be 0xFFFF.
- Code 65 ('A') with case-fold (→ 97 'a'): `trt_ascii[65]` should be 97.
- Code 127 with identity: `trt_ascii[127]` should be 0xFFFF.
- Out-of-range: a translation result of 300 should store 0xFFFF and fall back.
