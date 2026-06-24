# Emacs Patch Set

This repository collects local Emacs patches used for performance and memory
experiments.  They are intended for private builds and local evaluation.

All patches listed here were developed with LLM guidance.  Because of that
provenance, they cannot be submitted to or accepted by upstream Emacs.

## Patches

### `ns_color_cache_0001.patch`

Adds an `NSColor` cache for the macOS/NS port.

Emacs creates many `NSColor` objects while drawing glyphs.  This patch avoids
repeated allocation and conversion for colors that are already known, improving
UI responsiveness.  It is stable, non-invasive, and limited to the NS drawing
path.

### `memory_compact_0001.patch`

Adds malloc-zone compaction after garbage collection on macOS.

The patch asks the system allocator to release fully free pages back to the
operating system.  This can lower resident memory usage and improve cache
locality during long sessions.  It is stable and uses the public malloc-zone
pressure-relief API where available.

### `re_ascii_caches_0001.patch`

Adds an ASCII cache for regular-expression symbol lookup.

This improves regex-heavy paths where the lookup repeatedly targets ASCII
characters.  In font-locking scenarios, this has shown roughly a 20%
improvement.

### `re_ascii_translate_cache_0001.patch`

Adds a translation cache for case-folding regular-expression searches.

This improves scenarios that repeatedly translate ASCII characters during
case-folding search.  In those cases, the patch has shown improvements of up to
50%.

### `ascii_caches_combined_0001.patch`

Combines the two regex ASCII cache patches:

- `re_ascii_caches_0001.patch`
- `re_ascii_translate_cache_0001.patch`

These two patches conflict when applied separately, so this combined patch is
the version to use when both regex cache improvements are desired together.

## Notes

- The patches are maintained for local builds.
- They should be reviewed and benchmarked against the exact Emacs revision they
  are applied to.
- The reported performance improvements are scenario-specific and should be
  treated as local measurements rather than upstream-quality benchmark claims.
