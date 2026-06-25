# Status: Regex Syntax Cache — 256-Entry
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/regex-syntax-cache/variants/single-byte-256/implementation.patch
```

## Build

All platforms. The cache change is purely C; no Objective-C.

## Known Issues

- The patch shows the `re_pattern_buffer` struct field replacement as a
  context-approximate hunk; the exact line numbers must be located in the
  actual source.
- Callers that pass `char c` (signed) to `SYNTAX_TRU` should cast to
  `unsigned char` before the comparison to avoid sign-extension giving
  values >= 256 for byte codes 128-255 on platforms where `char` is signed.
- The `(unsigned)(c) < 256` guard is always true for `unsigned char` values;
  it serves as documentation and future-proofing if the type changes.

## Testing

Test unibyte behavior: set up a unibyte buffer with Latin-1 characters (codes
128-255), compile a word-boundary pattern, and verify `syntax_ascii_valid` is
true and `syntax_ascii[200]` returns the correct syntax code.
See `tests/regex-syntax-cache/` for shared test infrastructure.
