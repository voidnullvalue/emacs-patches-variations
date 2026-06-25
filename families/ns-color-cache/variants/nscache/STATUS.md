# Status: NSColor Cache — NSCache
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 < families/ns-color-cache/variants/nscache/implementation.patch
```

## Build

macOS only. Conflicts with all other ns-color-cache variants.
Requires adding init/teardown call sites (noted as placeholders in the patch).

## Known Issues

- `countLimit = 512` is advisory; NSCache may hold more or fewer entries.
  Do not rely on this for strict memory bounds.
- Under memory pressure, NSCache may evict recently-used entries; if this
  causes visible performance regression (cache miss spike), switch to a
  deterministic CLOCK or set-associative variant.
- `ns_nscache_teardown` releases the NSCache but not any outstanding
  caller-held pointers to evicted objects. Callers use the returned color
  within the same call frame; this is safe.

## Testing

See `tests/ns-color-cache/` for shared test infrastructure.
NSCache's eviction behavior is non-deterministic and cannot be directly
unit-tested without simulating memory pressure; focus tests on lookup/store
correctness when the cache has not evicted.
