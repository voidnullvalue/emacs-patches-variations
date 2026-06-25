# Emacs Performance Patch Atlas
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

This repository is a **defensive publication and reproducible performance research
atlas** for a set of Emacs performance optimizations and their design-space
variants.

Every patch, document, test, benchmark, and tool in this repository was produced
with substantial LLM assistance (Claude Code, claude-sonnet-4-6). See
[PROVENANCE.md](PROVENANCE.md) for the full provenance statement.

---

## Quick Navigation

| Document | Purpose |
|---|---|
| [PROVENANCE.md](PROVENANCE.md) | Full LLM provenance disclosure |
| [AUDIT.md](AUDIT.md) | Audit log: errors found, reconstructions made |
| [BASELINE.md](BASELINE.md) | Base Emacs revision evidence and patch application |
| [DESIGN_SPACE.md](DESIGN_SPACE.md) | Design axes and variant rationale |
| [manifest.json](manifest.json) | Machine-readable index of all patches |

---

## Optimization Families

### 1. NSColor Cache (`families/ns-color-cache/`)

Caches `NSColor` objects keyed by packed pixel value in `+colorWithUnsignedLong:`.
Eliminates per-glyph colorspace conversion on the NS/macOS drawing path.

**Variants (8 total):**
- `baseline/` — 32768-slot direct-mapped, Fibonacci hash, process-lifetime, never-evict
- `variants/open-addressing/` — 512-slot open-addressing with generation-based eviction
- `variants/per-frame/` — per-`EmacsView` `NSMutableDictionary`, frame lifetime
- `variants/counted-lru/` — 64-slot CLOCK-approximation LRU
- `variants/single-entry-hot-cache/` — 1-slot, unconditional replacement, explicit teardown
- `variants/two-way-set-associative/` — 256 sets × 2 ways, per-set CLOCK eviction
- `variants/nsmutable-dictionary/` — `NSMutableDictionary` keyed by `NSNumber`, unbounded
- `variants/nscache/` — `NSCache` with advisory `countLimit=512`, auto-eviction
- `variants/per-display-backend/` — dictionary attached to `NsDisplayInfo` lifecycle

### 2. Malloc Pressure Relief (`families/malloc-pressure-relief/`)

Calls `malloc_zone_pressure_relief(NULL, 0)` after GC to reclaim fully-free
pages on macOS (SYSTEM_MALLOC).

**Variants (7 total):**
- `baseline/` — synchronous, time-gated + threshold-gated, every-N-GCs fallback
- `variants/background-dispatch/` — async via GCD `QOS_CLASS_BACKGROUND` queue
- `variants/adaptive-threshold/` — EMA-learned freed-bytes threshold
- `variants/manual-lisp-primitive/` — user-callable `(malloc-zone-pressure-relief)` defun
- `variants/idle-triggered/` — deferred to Emacs idle loop via pending flag
- `variants/every-gc-control/` — unconditional after every GC (benchmark reference)
- `variants/freed-byte-goal/` — passes GC freed-byte count as goal argument
- `variants/effectiveness-backoff/` — exponential backoff after low-yield compactions

### 3. Regex ASCII Syntax Cache (`families/regex-syntax-cache/`)

Flat array caching syntax codes for ASCII characters in `re_pattern_buffer`,
replacing `syntax_property()` calls in word-boundary opcodes.

**Variants (6 total):**
- `baseline/` — per-pattern, eager compile-time fill, 128 entries, `!parse_sexp_lookup_properties` guard
- `variants/global-epoch/` — single global 128-byte array with epoch-counter invalidation
- `variants/lazy-no-presexp-guard/` — runtime text-property check, no compile-time guard
- `variants/single-byte-256/` — 256-entry cache covering full byte range 0-255
- `variants/sidecar-allocation/` — syntax cache in separately allocated sidecar struct
- `variants/syntax-table-generation/` — generation-counter invalidation via syntax.c mutation sites
- `variants/specialized-ascii-matcher/` — pre-match dispatch to dedicated ASCII matching path

### 4. Regex ASCII Translate Cache (`families/regex-translate-cache/`)

Flat array caching translation results in `re_pattern_buffer`,
replacing `char_table_translate()` calls in case-fold match paths.

**Variants (6 total):**
- `baseline/` — per-pattern, eager compile-time fill, `unsigned char` entries, 0-255 coverage
- `variants/lazy-bitmap/` — lazy fill with 256-bit per-entry valid bitmap
- `variants/wide-entry/` — `uint16_t` entries, lossless, 0-255 fast path
- `variants/ascii-128/` — 128-entry cache (ASCII only), saves 128 bytes in struct
- `variants/sidecar-allocation/` — translate cache in separately allocated sidecar
- `variants/identity-sentinel/` — `uint16_t` entries with `0xFFFF` identity sentinel
- `variants/table-generation/` — `EQ`-based translate-table identity validation

### 5. Regex Combined Cache (`families/regex-combined-cache/`)

Applies both regex caches (syntax + translate) in a single non-conflicting patch.

**Variants (6 total):**
- `baseline/` — two independent `bool_bf` validity flags
- `variants/unified-generation/` — single `uint32_t` generation counter for both
- `variants/search-path-hook/` — search.c hook skips re_compile_pattern on key match
- `variants/per-character-struct/` — Array-of-Structs with independent per-character validity
- `variants/packed-entry/` — syntax + translate packed into one `uint16_t` per character
- `variants/independently-lazy/` — independent lazy initialization per subcache
- `variants/combined-sidecar/` — both caches in one `xmalloc`'d sidecar object

---

## Repository Structure

```
archive/original/           Original patches, preserved verbatim
families/
  <family>/
    baseline/
      implementation.patch  Clean baseline patch
      metadata.yaml         Machine-readable metadata
      DESIGN.md             Design rationale
      STATUS.md             Apply/build/known-issues status
    variants/
      <variant>/            Same four files per variant
combinations/               Non-conflicting multi-family patches
benchmarks/                 Benchmark scripts
tests/                      Logic unit tests (no Emacs build required)
tools/                      Patch verification and audit scripts
docs/                       (reserved)
```

---

## Applying Patches

Most patches apply to the Emacs base identified in [BASELINE.md](BASELINE.md).
Reconstructed baselines (regex-syntax-cache, regex-combined-cache) use
`--fuzz=3`:

```sh
# Clean baselines (exact line numbers):
patch -p1 < families/ns-color-cache/baseline/implementation.patch
patch -p1 < families/malloc-pressure-relief/baseline/implementation.patch
patch -p1 < families/regex-translate-cache/baseline/implementation.patch

# Reconstructed baselines (approximate line numbers):
patch -p1 --fuzz=3 < families/regex-syntax-cache/baseline/implementation.patch
patch -p1 --fuzz=3 < families/regex-combined-cache/baseline/implementation.patch
```

Design-space variant patches (those with `index 0000000000` in the diff header)
are specifications, not directly applicable diffs.

---

## Running Tests

The unit tests require only a C compiler (and `clang` for the Objective-C test):

```sh
# Regex syntax cache logic
cc -o /tmp/t tests/regex-syntax-cache/test_syntax_cache.c && /tmp/t

# Regex translate cache logic
cc -o /tmp/t tests/regex-translate-cache/test_translate_cache.c && /tmp/t

# Adaptive threshold logic
cc -o /tmp/t tests/malloc-pressure-relief/test_threshold_logic.c && /tmp/t

# NSColor cache logic (macOS only)
clang -framework Foundation -o /tmp/t tests/ns-color-cache/test_cache_logic.m && /tmp/t
```

---

## Inventory

| | Count |
|---|---|
| Baseline implementations | 5 |
| Variants (round 1) | 11 |
| Variants (round 2, added 2026-06-25) | 22 |
| **Total variants** | **33** |
| Cross-family compositions | 2 |
| **Total patch artifacts** | **40** |

Rejected designs are documented in [docs/rejected-designs.md](docs/rejected-designs.md).

---

## Audit Summary

Two composition errors were found in the original patch set:

1. **`re_ascii_caches_0001.patch`** incorrectly contained the full `alloc.c`
   malloc-zone block and both regex caches (should be syntax-only).
2. **`ascii_caches_combined_0001.patch`** incorrectly contained the `alloc.c`
   and `nsterm.m` sections alongside the combined regex caches.

See [AUDIT.md](AUDIT.md) for full analysis and reconstruction notes.

---

## Notes

- Patches are maintained for local builds and experimental evaluation.
- Performance numbers from the original README are local scenario-specific
  measurements, not upstream-quality benchmarks.
- This repository does not contact Emacs maintainers or submit anything upstream.
- All LLM-assisted content is clearly marked.
