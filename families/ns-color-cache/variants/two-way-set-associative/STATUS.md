# Status: NSColor Cache — 2-Way Set-Associative
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 < families/ns-color-cache/variants/two-way-set-associative/implementation.patch
```

## Build

macOS only. Conflicts with all other ns-color-cache variants.
No additional includes or libraries required.

## Known Issues

- With only 2 ways, the CLOCK loop in `ns_sa_pick_way` iterates at most
  `NS_SA_WAYS * 2 = 4` times before the fallback fires. The fallback
  (evict hand position unconditionally) is correct but bypasses the
  second-chance logic; add a loop-count assertion in debug builds.
- `ns_sa_hand[s]` is only advanced inside `ns_sa_pick_way`; on very
  hot paths the hand may stay at way 0 if way 1 is always unoccupied.
  This is correct behavior but the `ns_sa_hand` array wastes 256 bytes
  if the cache never fills to both ways.

## Testing

See `tests/ns-color-cache/` for shared test infrastructure.
The set-associative logic can be tested by filling a set with two keys,
verifying both are returned on lookup, then inserting a third key and
verifying that one of the original two was evicted via CLOCK.
