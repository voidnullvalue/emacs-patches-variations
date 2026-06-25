# Status: Malloc Pressure Relief — Effectiveness Backoff
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/malloc-pressure-relief/variants/effectiveness-backoff/implementation.patch
```

## Build

macOS only. Single-file patch to alloc.c.

## Known Issues

- `backoff_gc_interval` is not exposed as `DEFVAR_INT`, so it cannot be
  inspected from Lisp. Add `DEFVAR_INT ("malloc-compact-backoff-interval", ...)`
  pointing to `backoff_gc_interval` if observability is needed.
- The threshold trigger (`freed_bytes >= BACKOFF_FREED_THRESHOLD`) fires
  independently of the backoff period. If a GC frees ≥ 4 MB, compaction runs
  regardless of the current interval. This is intentional: the freed-bytes
  trigger is about when there is likely returnable memory, the backoff is about
  avoiding wasted walks.
- `backoff_gc_counter` is reset to 0 before the arena walk. If the walk fails
  or is interrupted, the counter is still reset. This is acceptable: the next
  periodic trigger will fire after `backoff_gc_interval` more GCs.

## Testing

`backoff_update()` can be unit-tested directly:
```c
// After 3 low-yield calls:
backoff_update(0);  // streak=1
backoff_update(0);  // streak=2
backoff_update(0);  // streak=3 → interval doubles to 128
assert(backoff_gc_interval == 128);

// One meaningful release:
backoff_update(128 * 1024);  // resets
assert(backoff_gc_interval == 64);
```
