# Status: Regex Translate Cache — Wide Entry Variant
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/regex-translate-cache/variants/wide-entry/implementation.patch
```

## Build

Requires `<stdint.h>` for `uint16_t`. The patch adds `#include <stdint.h>` to
`regex-emacs.h`. If the header is already included elsewhere, remove the duplicate.

## Known Issues

- The sentinel check `<= 0xFFFF` in `TRANSLATE_TRU` is always true for a `uint16_t`
  value. It should be `!= 0xFFFF` to correctly force the slow path for astral
  Unicode translations. This is a correctness bug in the patch — fix before
  building.
- `uint16_t` alignment: `re_pattern_buffer` starts with `unsigned char *` so
  may have alignment padding before `trt16_ascii`. Verify struct layout.
- The `#include <stdint.h>` inside the `@interface`/struct context in the
  `.h` file may be unusual. Move it to the top of the file if the compiler
  rejects it in that position.

## Testing

See `tests/regex-translate-cache/` for translation correctness tests,
especially for codes in 128-255.
