# Status: Regex Combined Cache — Search-Path Hook Variant
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=5 < families/regex-combined-cache/variants/search-path-hook/implementation.patch
```

The search.c section uses placeholder comments (`/* ... original compile_pattern body ... */`)
because the exact line numbers and context of `compile_pattern` cannot be determined
without the actual source. The patch provides the logic structure; manual integration
with the actual `compile_pattern` function body is required.

## Build

Not tested. This patch modifies three files and requires careful integration with
the existing `compile_pattern` function in `search.c`.

## Known Issues

1. **In-place modification bug:** `modify-syntax-entry` and similar functions that
   mutate an existing syntax table object do not change the table's Lisp_Object
   pointer. The search-path cache will return stale results after such mutations.
   Fix: add an epoch counter or check a modification timestamp.

2. **`xfree` / `xmalloc` pattern:** The cache uses `xmalloc` for the pattern
   bytes buffer. If Emacs is built with a garbage-collecting allocator, ensure
   `sp_cache.pattern_bytes` is not a GC root.

3. **Thread safety:** `sp_cache` is a global with no locking. If Emacs gains
   multi-threaded search (possible with the native-thread branch), this cache
   must be thread-local or protected.

4. **`bufp` lifetime:** `sp_cache.bufp` holds a pointer into Emacs's pattern
   cache. If the pattern cache evicts the entry (e.g., `clear-regexp-cache`),
   the pointer becomes dangling. The fast path should verify that `bufp` is still
   valid by checking if it matches the current pattern cache entry.
