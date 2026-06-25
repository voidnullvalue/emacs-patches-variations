# Design: Malloc Pressure Relief — Every-GC Control
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

To evaluate whether threshold-gating and rate-limiting are effective optimizations,
we need a reference point: what happens if we remove all gating and call
`malloc_zone_pressure_relief` after every GC?

This variant is the answer. It is intentionally the simplest possible design —
no conditions, no counters, no time checks — and is explicitly marked as a
benchmark control, not a production recommendation.

Use it to measure:
- **Maximum RSS benefit:** If every-GC yields minimal additional benefit over
  the gated baseline, the threshold gate is already capturing most of the
  available improvement.
- **Maximum latency cost:** If every-GC adds X seconds of cumulative overhead
  per hour of use, that bounds how much the gated variants are saving.

## Algorithm

After every call to `garbage_collect()` completes, call:
```c
malloc_zone_pressure_relief(NULL, 0);
```
No threshold, no time check, no periodicity counter. Record invocation count,
bytes returned, and cumulative latency in microseconds.

## Counters

Three file-static counters accumulate across the session:
- `everygc_compact_count` — number of compaction calls.
- `everygc_bytes_returned` — total bytes reported released.
- `everygc_latency_us` — total wall-clock microseconds spent in compaction.

These can be read from a debugger (`p everygc_latency_us`) or exposed via
`DEFVAR_INT` for Lisp-level instrumentation.

## Expected Latency Profile

`malloc_zone_pressure_relief` walks the allocator arena. Empirically on macOS:
- Small heap (< 100 MB): 1–5 ms per call.
- Medium heap (100–500 MB): 5–20 ms per call.
- Large heap (> 500 MB): 10–50 ms per call.

A workload with one GC per second and a 200 MB heap would accumulate ~10 seconds
of compaction overhead per session hour. The gated baseline avoids most of this.

## Comparison Table

| Property | Baseline | Every-GC control | Adaptive threshold |
|---|---|---|---|
| Calls per 100 GCs | ~2 (gated) | 100 | ~5 (adaptive) |
| Latency overhead | Low | High | Very low |
| RSS benefit | Moderate | Maximum | Moderate-high |
| Purpose | Production | Benchmark reference | Production (adaptive) |
