# Status: Malloc Pressure Relief — Adaptive Threshold Variant
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/malloc-pressure-relief/variants/adaptive-threshold/implementation.patch
```

## Build

macOS only. No extra dependencies beyond the baseline.

## Known Issues

- The EMA uses integer arithmetic. `adaptive_ema * 4` could overflow if bytes
  released exceeds ~4.6 EB (`UINTMAX_MAX / 4`). In practice this cannot happen
  (physical RAM < 2^63 bytes), but adding a saturation check is defensive.
- After the periodic fallback fires, `gc_counter` resets to 0. The next periodic
  fire is 64 GCs later. This is the correct intended behavior.
- The threshold is not exposed as a `DEFVAR` variable, so users cannot inspect
  it from Lisp. If instrumentation is needed, add
  `DEFVAR_INT ("emacs-malloc-compact-threshold", ...)` pointing to `adaptive_threshold`.

## Testing

See `tests/malloc-pressure-relief/` for simulation-based threshold tests.
