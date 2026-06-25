# Design: Malloc Pressure Relief — Effectiveness Backoff
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The adaptive-threshold variant (EMA-based) adjusts when compaction fires based
on how much memory was freed by the preceding GC. This variant takes a different
approach: adjust how often compaction fires based on how much compaction itself
has historically returned.

If `malloc_zone_pressure_relief` consistently returns < 64 KB (below
`BACKOFF_LOW_YIELD_BYTES`), there is little benefit to calling it every 64 GCs.
Doubling the interval to 128, then 256, then capping at 512 GCs reduces the
arena-walk overhead on workloads where the allocator has few returnable pages.

When a compaction call returns a meaningful yield (≥ 64 KB), the interval resets
to 64 — the allocator now has returnable pages, and more frequent compaction is
again worthwhile.

## Backoff Algorithm

```
Initial: backoff_gc_interval = 64, ineffective_streak = 0

After each compaction:
  if released >= LOW_YIELD_BYTES:
    ineffective_streak = 0
    backoff_gc_interval = INITIAL_INTERVAL  (reset)
  else:
    ineffective_streak++
    if ineffective_streak >= N_INEFFECTIVE_MAX:
      ineffective_streak = 0
      backoff_gc_interval = min(backoff_gc_interval * 2, MAX_INTERVAL)
```

The GC interval progresses: 64 → 128 → 256 → 512 (cap).
Three consecutive low-yield compactions are required to advance each step.

## Overflow Safety

`backoff_gc_interval` is `unsigned int`. Doubling is guarded by:
```c
if (backoff_gc_interval <= BACKOFF_MAX_INTERVAL / 2)
    backoff_gc_interval *= 2;
else
    backoff_gc_interval = BACKOFF_MAX_INTERVAL;
```
This ensures no overflow for any value of `BACKOFF_MAX_INTERVAL`.

## Independent Testability

`backoff_update(size_t released)` is exported (non-static). Unit tests can call
it directly with synthetic released values and verify `backoff_gc_interval`
transitions without running a GC or Emacs.

## Comparison Table

| Property | Baseline | Adaptive threshold (EMA) | Effectiveness backoff |
|---|---|---|---|
| Adapts to | GC freed bytes | Compaction yield | Compaction yield |
| Algorithm | Static threshold | EMA moving average | Exponential interval doubling |
| Reset condition | N/A | Always updates | On meaningful yield |
| Interval range | Always 64 GCs | N/A (threshold-based) | 64–512 GCs |
| State | Counters only | EMA + threshold | Interval + streak |
| Testability | N/A | Internal | Exported update function |
