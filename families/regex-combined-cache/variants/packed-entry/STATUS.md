# Status: Combined Regex Cache — Packed Entry
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/regex-combined-cache/variants/packed-entry/implementation.patch
```

## Build

All platforms. Requires `<stdint.h>` for `uint16_t`.

## Known Issues

- Add a static assertion: `_Static_assert((int)Smax <= (int)RE_PACK_SYNTAX_MASK, ...)`
  to catch future syntax code additions that exceed 5 bits.
- The `|=` assignment during population assumes `re_packed[i]` starts at 0
  (from `memset`). If `re_packed` is not zeroed before population, `|=` may
  set spurious bits from a previous compile. The patch includes `memset` for this.
- `RE_PACKED_TRT(e)` right-shifts and masks with 0xFF; result is `unsigned int`.
  Cast to `int` before returning from `TRANSLATE_TRU` to match `TRANSLATE`'s type.
- Reserved bit 15 must be 0. Verify that no population code sets bit 15.

## Testing

- Verify bit layout constants: `RE_PACK_SYNTAX_VALID == 0x20`,
  `RE_PACK_TRT_VALID == 0x4000`, etc.
- Code 65 ('A') with syntax Sword (1) and translate 97 ('a'):
  `re_packed[65]` should equal `(1 << 0) | (1 << 5) | (97 << 6) | (1 << 14)`.
- Syntax code that exceeds 5 bits: verify valid bit is clear.
- Translate result > 255: verify translate_valid bit is clear.
