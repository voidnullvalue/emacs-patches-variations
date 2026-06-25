# Status: NSColor Cache — NSMutableDictionary
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 < families/ns-color-cache/variants/nsmutable-dictionary/implementation.patch
```

## Build

macOS only. Conflicts with all other ns-color-cache variants.
Requires adding `ns_color_dict_init()` and `ns_color_dict_teardown()` calls
at the correct sites; the patch notes these as placeholder line numbers.

## Known Issues

- `@((unsigned long)c)` generates an autoreleased NSNumber for large values
  outside the shared-integer pool range. If called outside an active autorelease
  pool, add an explicit `@autoreleasepool { ... }` wrapper.
- The dictionary is not bounded. Monitor dictionary count in profiling builds
  if used with workloads that generate many distinct colors.
- `NSMutableDictionary` is not thread-safe; do not call from background threads.

## Testing

Test by inserting 3 distinct keys, verifying all are retrieved, then checking
that re-insertion with the same key returns the same object.
See `tests/ns-color-cache/` for shared test infrastructure.
