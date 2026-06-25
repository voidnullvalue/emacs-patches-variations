# Status: Malloc Pressure Relief — Idle-Triggered
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/malloc-pressure-relief/variants/idle-triggered/implementation.patch
```

## Build

macOS only for the pressure relief execution. The flag and idle-path call
compile on all platforms (the non-macOS stub returns false immediately).
Modifies two files (alloc.c and keyboard.c).

## Known Issues

- The keyboard.c hunk line number is a placeholder; locate the correct idle
  processing section in the actual source (typically inside a block guarded by
  `!detect_input_pending()`).
- If Emacs is never idle (e.g., running an infinite loop with `(while t ...)`),
  `execute_pending_pressure_relief` is never called. Add `(malloc-zone-pressure-relief)`
  to an appropriate hook if continuous background processing is expected.
- The `IDLE_PRESSURE_FREED_THRESHOLD` constant matches the baseline (4 MB).
  Adjust if the workload's GC freed amounts differ significantly.

## Testing

- `maybe_schedule_idle_pressure_relief(0)` should not set the flag.
- `maybe_schedule_idle_pressure_relief(5 * 1024 * 1024)` should set the flag.
- Calling it again with the flag set should be a no-op.
- `execute_pending_pressure_relief()` with flag false should return false.
- `execute_pending_pressure_relief()` with flag true should clear flag, call
  relief, and return true.
