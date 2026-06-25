# Status: Malloc Pressure Relief — Freed-Byte Goal
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/malloc-pressure-relief/variants/freed-byte-goal/implementation.patch
```

## Build

macOS only. Single-file patch to alloc.c.

## Known Issues

- The `goal` argument behavior (early termination) is not documented as
  guaranteed by Apple's allocator.  Do not assume latency improvement without
  profiling the specific macOS version and heap size in question.
- When the periodic trigger fires and `freed_bytes == 0`, `goal = 0` (maximal)
  is used, which is identical to the baseline.  The non-zero goal is only
  available when a GC actually freed measurable memory.
- `freed_goal_bytes_returned` tracks total bytes returned across all calls;
  compare against `freed_goal_compact_count` to compute per-call averages.

## Testing

Unit-testable via policy-level tests (no Emacs build required):
- `compact_with_freed_goal(0)` on first call — should not fire (below threshold).
- `compact_with_freed_goal(5 * 1024 * 1024)` — should fire and call
  `malloc_zone_pressure_relief(NULL, 5242880)`.
- Verify SIZE_MAX clamp: pass `UINTMAX_MAX` as `freed_bytes` — goal should
  be `SIZE_MAX`, not truncated.
