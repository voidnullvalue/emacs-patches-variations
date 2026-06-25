# Design: Malloc Pressure Relief — Idle-Triggered
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline invokes `malloc_zone_pressure_relief` synchronously at the end of
`garbage_collect()`. The arena walk takes 1–50 ms on large heaps, adding latency
to every qualifying GC pause. The background-dispatch variant moves the walk to
a GCD thread, adding concurrency complexity.

The idle-triggered variant takes a middle path: set a lightweight flag during
GC, then execute the arena walk on the next Emacs idle iteration (when the event
loop has no pending input). This guarantees zero GC latency while avoiding the
threading complexity of GCD.

## Architecture

```
garbage_collect()
  → maybe_schedule_idle_pressure_relief(freed_bytes)
     if !pending && freed_bytes >= threshold:
       pressure_relief_pending = true
     (returns immediately — no arena walk)

Emacs event loop (idle)
  → execute_pending_pressure_relief()
     if !pending: return false
     pending = false            ← cleared BEFORE walk (re-entrant-safe)
     malloc_zone_pressure_relief(NULL, 0)
     return true
```

## Pending Flag Semantics

`pressure_relief_pending` is a single bool:
- **Set** by `maybe_schedule_idle_pressure_relief` when freed_bytes ≥ threshold.
- **Cleared** by `execute_pending_pressure_relief` before the arena walk begins.
- **Not duplicated:** if already true, subsequent GCs skip the set operation.

Clearing before the walk (not after) means a GC that fires during the walk
can re-set the flag, scheduling another idle execution. This is the correct
behavior: the GC freed more memory after compaction began.

## Integration Point

`execute_pending_pressure_relief` is called from the Emacs idle processing
section in `keyboard.c`. Emacs has an established pattern for deferred idle
work at this location (e.g., `blink-cursor-timer`). The call is unconditional
but O(1) when `pressure_relief_pending` is false (one bool check, one branch).

## Shutdown

If Emacs exits while `pressure_relief_pending = true`, the arena walk never
runs. This is intentional: the OS reclaims all process memory on exit anyway.

## Comparison Table

| Property | Baseline | Background dispatch | Idle-triggered |
|---|---|---|---|
| GC latency impact | Small (synchronous) | Zero (async) | Zero (deferred) |
| Threading | None | GCD background | None |
| Execution timing | During GC | Async, unpredictable | Next idle iteration |
| Complexity | Low | Medium | Low |
| Files modified | 1 | 1 | 2 (alloc.c + keyboard.c) |
