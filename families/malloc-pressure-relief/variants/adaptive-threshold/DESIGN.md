# Design: Malloc Pressure Relief — Adaptive Threshold Variant
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline's `MEMORY_COMPACT_FREED_THRESHOLD = 4 MB` is a guess. In some
workloads (e.g., loading many packages), individual GCs free far more than 4 MB
and compaction consistently yields significant page returns. In other workloads
(e.g., steady typing in a single buffer), individual GCs free very little and
compaction yields nearly nothing.

A static threshold cannot be simultaneously optimal for both: too low and it
runs compaction on small GCs where the yield is near zero; too high and it misses
opportunities to compact after large GCs that actually free substantial memory.

## Algorithm

After each compaction, record how many bytes were released (`malloc_zone_pressure_relief`
return value). Update a running EMA:

```
ema = (alpha * released) + ((1 - alpha) * ema)
```

with alpha = 1/4 (integer arithmetic: `ema = (1 * released + 3 * ema) / 4`).

Set the new threshold equal to the updated EMA, clamped to `[EMA/2, EMA*4]` and
floored at 256 KB.

**Intuition:** If compactions consistently yield X bytes, set the threshold to X.
This means "compact when freed ≥ what compaction typically recovers." This is a
proportionality policy: compact when a GC freed as much memory as compaction has
recently reclaimed.

When yield is high (allocator has many free pages to return), the threshold also
rises — requiring a large GC before triggering compaction, since the walk cost is
proportional to the number of pages examined.

When yield is low (allocator has few free pages to release), the threshold drops
— allowing compaction even after modest GCs, since the walk is cheap and the
cleanup is minor but cumulative.

## EMA Parameters

Alpha = 0.25: gives approximately 75% weight to the past 4 samples, balancing
responsiveness (quick adaptation to changed workload) and stability (not reacting
to individual spiky GCs).

Clamp to `[EMA/2, EMA*4]`: bounds oscillation. Without clamping, a single
zero-yield compaction would set the threshold to 0 (compact always), and a single
huge-yield compaction would set it to the full yield (compact very rarely).

Floor at 256 KB: ensures the threshold never drops so low that every tiny GC
triggers a compaction.

## Comparison with Baseline

| Property | Baseline | This variant |
|---|---|---|
| Threshold | Static 4 MB | Adaptive (EMA of yield) |
| Tuning required | Yes (hardcoded constant) | No (self-tunes) |
| Response to low-yield workloads | May compact unnecessarily | Raises threshold |
| Response to high-yield workloads | May miss compactions | Lowers threshold |
| Extra state | None | EMA accumulator + threshold variable |
