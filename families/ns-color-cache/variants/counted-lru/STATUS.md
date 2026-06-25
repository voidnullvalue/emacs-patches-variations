# Status: NSColor Cache — CLOCK-LRU Variant
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 < families/ns-color-cache/variants/counted-lru/implementation.patch
```

## Build

Not tested. Conflicts with baseline and other ns-color-cache variants.

## Known Issues

- The `ns_lru_lookup` function is O(N). For large N (if `NS_COLOR_LRU_SIZE` is
  increased significantly), add a hash index alongside the linear array.
- `ns_lru_count` is not decremented on eviction (entries are reused, not removed).
  This is correct but the count is misleading after the table fills; rename to
  `ns_lru_initialized` or add a comment.
- The CLOCK loop is unbounded in theory; in practice it terminates in at most N
  iterations because clearing all `ref` bits guarantees an eviction candidate.

## Testing

See `tests/ns-color-cache/` for shared test infrastructure.
