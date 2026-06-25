# Status: NSColor Cache — Single-Entry Hot Cache
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 < families/ns-color-cache/variants/single-entry-hot-cache/implementation.patch
```

## Build

macOS only. Conflicts with all other ns-color-cache variants.
Requires adding a `ns_hot_cache_teardown()` call to the NS terminal
shutdown path (`ns_term_shutdown` in nsterm.m).

## Known Issues

- The teardown call site (`ns_term_shutdown`) is noted as a placeholder line number
  in the patch; the exact line must be located in the actual source.
- If `+colorWithUnsignedLong:` is ever called from a background thread, the static
  state requires a lock. The current NS drawing path is single-threaded.
- The hit rate degrades to exactly 50% for any workload with exactly two
  alternating colors; profile before assuming benefit.

## Testing

See `tests/ns-color-cache/` for shared test infrastructure.
The single-entry logic can be unit-tested without Emacs by exercising
`ns_hot_cache_lookup` and `ns_hot_cache_store` directly.
