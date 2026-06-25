# Status: NSColor Cache Baseline
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 < families/ns-color-cache/baseline/implementation.patch
```

## Build

Not tested in this repository. Requires an Emacs source tree at the correct
base revision. See BASELINE.md for blob hash identification.

## Known issues

None identified in review.

## Performance Notes

Original README reports improved UI responsiveness for NS glyph drawing.
No quantitative benchmark is recorded. The improvement is proportional to how
often `+colorWithUnsignedLong:` is called with the same value; in font-lock
heavy buffers with many faces the gain should be measurable.
