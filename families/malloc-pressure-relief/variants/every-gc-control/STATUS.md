# Status: Malloc Pressure Relief — Every-GC Control
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/malloc-pressure-relief/variants/every-gc-control/implementation.patch
```

## Build

macOS only. The `#ifdef HAVE_MALLOC_MALLOC_H` guard compiles the call on macOS
and elides it on other platforms. No extra libraries required.

## Known Issues

- This variant intentionally adds substantial latency to every GC.  Do not
  run benchmarks measuring GC throughput with this variant active unless
  that is the intended measurement.
- `everygc_latency_us` saturates at `UINTMAX_MAX` on extremely long sessions;
  in practice the 64-bit counter will not overflow in a normal session.
- The `timespec_sub` / `current_timespec` calls add ~100 ns overhead per
  compaction call; acceptable for benchmarking, not zero-cost.

## Testing

- After one GC, `everygc_compact_count` should be 1.
- `everygc_latency_us` should be > 0 after any GC on macOS.
- On non-macOS, `compact_after_every_gc` is never compiled in.
