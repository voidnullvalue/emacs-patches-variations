# Status: Malloc Pressure Relief Baseline
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 < families/malloc-pressure-relief/baseline/implementation.patch
```

## Build

macOS only. Requires `HAVE_MALLOC_MALLOC_H` to be defined (standard on macOS
Emacs builds). The patch is a no-op on Linux/Windows.

## Known Issues

- `MALLOC_PROBE` macro usage at line 127 of the patch: verify this macro is
  defined in the target Emacs revision (it may be `malloc_probe` in some
  versions). Adjust if the build fails.
- The patch increments `gc_counter` unconditionally even on the rate-limited path
  (counter never resets to 0 after the periodic trigger fires). This means after
  the first periodic GC, subsequent ones that cross the threshold will also be
  periodic triggers simultaneously. The behavior is correct but slightly wasteful.

## Performance Notes

Original README notes "can lower resident memory usage and improve cache locality
during long sessions." Effectiveness depends on workload; benchmarks needed.
