# Status: NSColor Cache — Per-Display Backend
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 < families/ns-color-cache/variants/per-display-backend/implementation.patch
```

## Build

macOS only. Conflicts with all other ns-color-cache variants.
Requires adding `NSMutableDictionary *color_cache` to `struct ns_display_info`
in `src/nsterm.h` (not reflected in the patch above which focuses on nsterm.m).

## Known Issues

- `ns_display_info_for_frame(selected_frame)` may return NULL in pathological
  states (no selected frame, startup, shutdown). The patch guards with
  `if (dpyinfo && dpyinfo->color_cache)`, which gracefully falls back to
  creating the color without caching.
- The patch marks line numbers for init/teardown as placeholders; locate the
  exact sites in `ns_initialize_display_info` and the display teardown path.
- If `+colorWithUnsignedLong:` is called before `ns_initialize_display_info`
  completes, `dpyinfo->color_cache` may be nil; the guard handles this.

## Testing

Test by creating two display-info instances, storing different colors in each,
and verifying that lookup on one does not return entries from the other.
Teardown test: free one display info and verify its color_cache is nil.
