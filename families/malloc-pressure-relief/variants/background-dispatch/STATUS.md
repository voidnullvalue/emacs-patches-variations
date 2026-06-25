# Status: Malloc Pressure Relief — Background Dispatch Variant
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/malloc-pressure-relief/variants/background-dispatch/implementation.patch
```

## Build

macOS only. Requires `dispatch/dispatch.h` (standard on macOS 10.6+).
Link with no extra flags — libdispatch is part of the system.

## Known Issues

- Thread-safety of `malloc_zone_pressure_relief` from a background thread is an
  assumption. Verify with TSan or by examining libsystem_malloc source.
- The GC state (e.g., `gcs_done`, `tot_before`) is NOT captured in the dispatch
  block (the block captures nothing). The block only calls the allocator API,
  which is correct. Do not add Lisp/GC state access to the block.
- If Emacs is quitting while a background compaction is in flight, the block
  will continue to completion (GCD retains the queue). This is harmless but
  may delay process exit slightly.
- `last_dispatch` and `last_dispatch_set` are file-static and accessed from the
  main thread only (the gate check runs before dispatch). This is correct.
