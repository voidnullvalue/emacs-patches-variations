# Status: NSColor Cache — Open-Addressing Variant
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 < families/ns-color-cache/variants/open-addressing/implementation.patch
```

This variant conflicts with the baseline and other ns-color-cache variants.
Apply only one at a time.

## Build

Not tested. macOS NS port only.

## Known Issues

- The generation counter (`uintmax_t`) wraps after 2^64 insertions. In practice
  this never happens (18 quintillion insertions per session). If wrap-safety
  is required, use modular comparison.
- `NsColorEntry` struct is declared in the .m file rather than a header.
  If other .m files need access to the cache, move the struct to a header.

## Testing

See `tests/ns-color-cache/` for a test harness that runs the cache logic
under simulated color sequences.
