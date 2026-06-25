# Design: NSColor Cache — Per-Display Backend
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

All other variants in this family use process-global state. When Emacs is used
with multiple displays (extended desktop, mirrored display, or dynamic connection
and reconnection), global state presents two problems:

1. **Cross-display contamination:** Cached colors for display A may be returned
   when rendering for display B, even if the two displays have different
   colorspaces or gamma. (In practice, NS normalizes colors before packing, but
   the principle is sound.)

2. **Stale cache after disconnect:** When a display is disconnected and its
   `NsDisplayInfo` is destroyed, globally-cached `NSColor` objects for that
   display remain in the process cache with dangling display associations.

Attaching the cache to `NsDisplayInfo` solves both: the cache is created when
the backend starts and released when it is torn down.

## Algorithm

**Field added to `NsDisplayInfo`:** `NSMutableDictionary *color_cache;`

**Initialization** (`ns_initialize_display_info`): Allocate the dictionary and
assign to `dpyinfo->color_cache`.

**Lookup/store**: Access `dpyinfo->color_cache` via the display info for the
current frame (`ns_display_info_for_frame(selected_frame)`).

**Teardown** (display-specific teardown, `ns_term_shutdown`):
`[dpyinfo->color_cache release]; dpyinfo->color_cache = nil`.

## Distinction from Per-Frame Cache

`EmacsView` (per-frame) represents a single window. `NsDisplayInfo` represents
the connection to a physical display device. One display may have many frames.
This cache is shared by all frames on the same display (correct behavior), while
still being isolated from other displays.

The existing `per-frame` variant uses an `NSMutableDictionary` attached to
each `EmacsView`. The per-display variant is coarser-grained: the dictionary is
shared across all windows on the same display, reducing duplicate entries for
themes with fixed color sets.

## Integration Point

The class method `+colorWithUnsignedLong:` must access the current
`NsDisplayInfo`. This is achieved via `ns_display_info_for_frame(selected_frame)`
— the same pattern used elsewhere in nsterm.m for display-specific operations.

## Comparison Table

| Property | Global (baseline) | Per-frame | Per-display (this) |
|---|---|---|---|
| Scope | Process | One EmacsView | One display/backend |
| Lifetime | Process | Frame lifetime | Backend lifetime |
| Multi-display isolation | No | Yes | Yes |
| Distinct dictionaries | 1 | N (one per frame) | D (one per display) |
| Release on disconnect | No | With frame | Yes |
