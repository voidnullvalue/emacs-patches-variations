# Design: NSColor Cache — Per-Frame Variant
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline uses a global cache shared across all frames. This causes a subtle
correctness risk: an `NSColor` created for frame A (which may be on a P3
wide-gamut display) and cached globally could be returned for frame B (on a
standard sRGB display). Whether `NSColor` objects are display-specific depends on
the colorspace they were created with; `colorForEmacsRed:green:blue:alpha:` may
or may not account for the current display context.

Beyond correctness, the global cache never releases memory. A frame that was open
for an hour and displayed a rich theme (100+ colors) keeps all those NSColors live
for the entire session even after the frame is closed.

## Approach

Add `NSMutableDictionary *colorCache` to the `EmacsView` ivar block. The dictionary
maps `@(pixel_value)` (an `NSNumber`) to `NSColor*`. It is allocated lazily on first
use and released in `-dealloc`.

The lookup path in `+colorWithUnsignedLong:` retrieves the current focus view via
`[NSView focusView]` and checks if it is an `EmacsView`. If so, it checks the
per-frame cache. On miss, it creates the color and stores it in the frame's
dictionary.

## Why NSMutableDictionary

A hash map is the natural Cocoa structure here. The key set per frame is bounded
(< a few hundred colors for any theme), so the hash map stays small and dictionary
lookup is fast. The NSNumber boxing allocates objects, but this overhead is paid
only on the miss path (first use of a color per frame), amortized over subsequent
hits.

An alternative would be to embed a C array in `EmacsView` (as in the baseline
but per-instance). This would match the baseline's lookup speed but requires a
fixed maximum size or dynamic allocation of the ivar.

## Lifetime and Invalidation

The dictionary's lifetime is tied to the `EmacsView` instance:
- Allocated lazily in `ns_frame_color_store` on first miss.
- Released and nilled in `-dealloc`.

No explicit invalidation is needed. When the frame changes display (display
migration), Emacs typically redraws from scratch, causing the colors to be
looked up and re-created (from the existing frame dictionary). For full
correctness on display migration, the dictionary should be flushed; this is
a known limitation.

## Comparison with Baseline

| Property | Baseline | This variant |
|---|---|---|
| Cache lifetime | Process | Frame |
| Cache scope | Global | Per-frame |
| Memory freed on frame close | No | Yes |
| Multi-display correctness | Risk | Isolated |
| Lookup overhead | Array index | ObjC message + hash |
