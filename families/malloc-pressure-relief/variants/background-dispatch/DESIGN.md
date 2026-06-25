# Design: Malloc Pressure Relief — Background Dispatch Variant
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline calls `malloc_zone_pressure_relief(NULL, 0)` synchronously at the
end of `garbage_collect()`. On a session with substantial fragmentation, this
walk can take tens of milliseconds, adding a perceptible pause to what is already
an already-disruptive GC event.

This variant eliminates the GC-time latency by dispatching the compaction to a
dedicated GCD serial queue at `QOS_CLASS_BACKGROUND` priority. The GC completes
immediately; compaction runs in the background while Emacs continues executing.

## Mechanism

**Queue creation:** A singleton `dispatch_queue_t` is created once via
`dispatch_once`. It is a serial queue targeted at the OS background queue, so
it runs at the lowest OS scheduling priority and is preempted by all other work.

**Dispatch:** `dispatch_async` submits an Objective-C block (or C11 closure)
that calls `malloc_zone_pressure_relief(NULL, 0)` and then clears the in-flight
flag.

**Overlap prevention:** `bgcompact_in_flight` is an `_Atomic int`. Before
dispatching, `atomic_compare_exchange_strong` sets it from 0 to 1. If it was
already 1 (prior dispatch in flight), the new request is dropped. The block clears
it to 0 on completion. This ensures at most one compaction runs at a time.

## Thread Safety of `malloc_zone_pressure_relief`

Apple's documentation for `malloc_zone_pressure_relief` does not explicitly state
it is thread-safe, but it is implemented in libsystem_malloc as a locked operation.
The macOS VMKit uses a per-zone lock during pressure relief. Concurrent `malloc`
and `free` calls from the main thread are safe because they also hold per-zone
locks (not the same lock held during zone enumeration, but the implementation
handles re-entrancy).

This is an implementation assumption that should be validated with `libsystem_malloc`
source or TSan testing.

## Rate Limiting

Even though dispatch is cheap, the compaction itself consumes CPU. The 5-second
minimum interval between dispatches prevents thrashing on workloads that trigger
many GCs in rapid succession (e.g., during byte-compilation).

The freed threshold is lowered to 1 MB (vs baseline's 4 MB) because the cost of
dispatching is near-zero; smaller GCs that free even a little heap are worth a
background compaction pass.
