# Status: Combined Regex Cache — Per-Character Struct
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/regex-combined-cache/variants/per-character-struct/implementation.patch
```

## Build

All platforms. Modifies regex-emacs.c and regex-emacs.h.

## Known Issues

- `sizeof(ReCharEntry)` depends on compiler ABI; on typical LP64 with bool as 1
  byte, the struct is `{u8 + u8 + bool + bool} = 4 bytes`. Add a static assertion:
  `_Static_assert(sizeof(ReCharEntry) == 4, "unexpected ReCharEntry padding")`.
- The outer `re_char_cache_valid` flag is redundant with the per-entry valid bits
  but serves as a fast early-exit when the cache is completely uninitialised.
- `memset(re_char_cache, 0, ...)` works correctly only if `false` is represented
  as 0 (guaranteed by C99 §6.2.5 for _Bool and §6.3.1.2 for int-to-bool).

## Testing

- After compile with syntax: `re_char_cache[65].syntax_valid == true`,
  `re_char_cache[65].syntax == syntax_property(65, 0)`.
- After compile with nil translate: `re_char_cache[65].translate_valid == false`.
- After compile with translate: `re_char_cache[65].translate_valid == true`.
- Partial invalidation test: clear translate_valid for one entry; verify syntax
  for that entry still hits the fast path.
