# Status: NSColor Cache — Per-Frame Variant
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 < families/ns-color-cache/variants/per-frame/implementation.patch
```

Conflicts with baseline and other ns-color-cache variants.

## Build

Not tested. Requires locating the exact `EmacsView` ivar block and `-dealloc`
method in `nsterm.m`. The patch uses approximate line numbers (the ivar block
at ~line 320 and dealloc at ~line 910 will need adjustment based on actual source).

## Known Issues

- `[NSView focusView]` is documented as returning `nil` outside of `lockFocus` /
  `drawRect:` calls. If `+colorWithUnsignedLong:` is called outside a draw context,
  the per-frame cache is bypassed (colors fall through to `colorForEmacsRed:` as
  if no cache existed). This is safe but inefficient.
- Display migration (`NSWindowDidChangeScreenNotification`) does not flush the
  cache in this implementation. For correct behavior across display changes, add
  a notification handler that sends `[colorCache removeAllObjects]`.

## Testing

See `tests/ns-color-cache/` for a standalone harness.
