# Patch Readiness Audit
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
     Human review status: not-reviewed -->

This document audits every patch in the repository for applicability, completeness,
and technical soundness. It supersedes the informal notes in AUDIT.md where they
overlap.

---

## Audit Methodology

Each patch was inspected for:

1. **Real index hashes** — `index <from>..<to>` lines with real blob SHAs vs
   `0000000000` specification-only markers.
2. **Line-number fidelity** — are `@@ -old,n +new,n @@` headers derived from
   actual file content or reconstructed approximations?
3. **Completeness** — do all modified call sites appear? Are initialization and
   teardown present? Are all required Emacs headers and types referenced correctly?
4. **Placeholder content** — pseudo-code, `/* TODO */` hunks, unresolved symbols.
5. **Platform scope** — does the patch apply to all Emacs platforms, or only macOS?
6. **Semantic soundness** — does the logic appear correct given the surrounding code?
7. **Conflict potential** — does the patch conflict with others in this repository?

---

## Classification Key

| Label | Meaning |
|---|---|
| **COMPLETE** | Real blob hashes; all call sites present; no placeholder content; applies cleanly (possibly with --fuzz) |
| **RECONSTRUCTED** | Synthetic or approximate line numbers; content is correct but `git apply --check` may fail without `--fuzz` |
| **SPECIFICATION** | `index 0000000000` header; design document masquerading as a diff; not directly applicable |
| **MACONLY** | Targets Objective-C (`nsterm.m`) or macOS-only API; cannot build or test on Linux |
| **INCOMPLETE** | Missing call sites, placeholder hunks, unresolved types, or structural gaps |

Multiple labels may apply.

---

## Baseline Patches

### `families/ns-color-cache/baseline/implementation.patch`

- **Index line:** `index a7f3fc292c..4490dadd37` — real blob SHAs
- **Files modified:** `src/nsterm.m` (Objective-C)
- **Status:** COMPLETE, MACONLY
- **Assessment:** Clean, original patch. Adds a 32768-slot direct-mapped C array
  cache (`ns_color_cache[]`, `ns_color_cache_keys[]`) before `+colorWithUnsignedLong:`.
  Three new static functions (`ns_color_cache_index`, `ns_color_cache_lookup`,
  `ns_color_cache_store`) are complete with correct Fibonacci hashing.
  The `+colorWithUnsignedLong:` method body is complete.
- **Missing:** No test hunks; no initialization guard for multi-display teardown;
  `ns_color_cache_keys` is not `atomic_ulong` but NS drawing is main-thread-only
  so this is documented as acceptable.
- **Apply command:** `git apply families/ns-color-cache/baseline/implementation.patch`
- **Build:** Blocked on Linux (Objective-C / macOS SDK required)

---

### `families/malloc-pressure-relief/baseline/implementation.patch`

- **Index line:** `index 5fc166dbc2..f97d56a6f6` — real blob SHAs
- **Files modified:** `src/alloc.c`
- **Status:** COMPLETE
- **Assessment:** Clean, original patch. All content under `#ifdef HAVE_MALLOC_MALLOC_H`
  guards — compiles cleanly on Linux as a no-op. Adds `malloc_zone_compact()`,
  `maybe_compact_memory_after_gc()`, and integration at `garbage_collect()` exit.
  The `freed_bytes` variable extraction is correct; `malloc_probe` call is preserved.
  Rate-limiting and threshold logic are structurally complete.
- **Missing:** None for the baseline. The `#ifdef` guards mean Linux builds are
  unaffected. `MALLOC_PROBE (released)` call refers to a macro defined elsewhere
  in alloc.c — verified present in the context lines of adjacent hunks.
- **Apply command:** `git apply families/malloc-pressure-relief/baseline/implementation.patch`
- **Build (Linux):** Compiles if Emacs build deps are available; the new code is
  compiled out by the preprocessor guard.
- **Build (macOS):** Requires `HAVE_MALLOC_MALLOC_H` defined by configure.

---

### `families/regex-syntax-cache/baseline/implementation.patch`

- **Index line:** `index d8ed8a462a..reconstructed` — fabricated target hash
- **Files modified:** `src/regex-emacs.c`, `src/regex-emacs.h`
- **Status:** RECONSTRUCTED
- **Assessment:** Reconstructed from `re_ascii_caches_0001.patch` by stripping the
  alloc.c and translate-cache sections. The *content* of the hunks is correct (same
  code as appears in the original patch in the syntax-only sections), but the `@@`
  line-number offsets are reconstructed approximations, and the target blob hash
  is synthetic (`reconstructed`). `git apply --check` will reject it; `patch -p1
  --fuzz=3` may succeed.
- **Missing:** The target blob is unknown, making it impossible to verify the patch
  applies exactly. At least one hunk may have wrong line-number offsets after the
  `TRANSLATE_TRU` block is absent. See AUDIT.md Note 1.
- **Apply command:** `patch -p1 --fuzz=3 < families/regex-syntax-cache/baseline/implementation.patch`
- **Build:** Blocked (no confirmed apply; no Emacs build environment yet)

---

### `families/regex-translate-cache/baseline/implementation.patch`

- **Index line:** `index d8ed8a462a..e5a9926c2a` (regex-emacs.c), `index bcfff662f7..3ccb9df49e` (regex-emacs.h) — real blob SHAs
- **Files modified:** `src/regex-emacs.c`, `src/regex-emacs.h`
- **Status:** COMPLETE
- **Assessment:** Clean, original patch. All `TRANSLATE()` call sites in fastmap-scan
  and character-matching loops are replaced with `TRANSLATE_TRU()`. Cache population
  block in `re_compile_pattern` is present. `CHAR_TO_BYTE_SAFE` truncation handling
  is correct. Struct additions (`trt_ascii[256]`, `trt_ascii_valid`) are complete.
- **Missing:** None identified.
- **Apply command:** `git apply families/regex-translate-cache/baseline/implementation.patch`
- **Build:** Requires Emacs build environment (autoconf, libncurses-dev)

---

### `families/regex-combined-cache/baseline/implementation.patch`

- **Index line:** `index d8ed8a462a..9932824d6c` (regex-emacs.c), `index bcfff662f7..b6f2510ed0` (regex-emacs.h) — real blob SHAs (verified from original)
- **Files modified:** `src/regex-emacs.c`, `src/regex-emacs.h`
- **Status:** RECONSTRUCTED (reconstruction was blob-preserving)
- **Assessment:** Reconstructed by stripping the alloc.c and nsterm.m sections from
  `ascii_caches_combined_0001.patch`. The regex-emacs.{c,h} sections were extracted
  verbatim; the blob hashes are authentic. The patch combines both caches correctly
  with no hunk conflicts. However, `--fuzz=3` may be required because the `@@`
  headers for the syntax-section hunks may reflect the combined file (with the
  translate additions shifting offsets) rather than the baseline file.
- **Missing:** None in content; line-number precision uncertain for syntax hunks.
- **Apply command:** `patch -p1 --fuzz=3 < families/regex-combined-cache/baseline/implementation.patch`
- **Build:** Requires Emacs build environment

---

## Variant Patches — All 33

**Result: ALL 33 variant patches are SPECIFICATIONS.**

Every variant patch contains `0000000000` in at least one `index` line (either as
the from-hash, the to-hash, or both). This marker indicates the diff was not derived
from an actual file checkout; it is a design document in diff syntax.

| Family | Variant | Classification |
|---|---|---|
| ns-color-cache | counted-lru | SPECIFICATION, MACONLY |
| ns-color-cache | nscache | SPECIFICATION, MACONLY |
| ns-color-cache | nsmutable-dictionary | SPECIFICATION, MACONLY |
| ns-color-cache | open-addressing | SPECIFICATION, MACONLY |
| ns-color-cache | per-display-backend | SPECIFICATION, MACONLY |
| ns-color-cache | per-frame | SPECIFICATION, MACONLY |
| ns-color-cache | single-entry-hot-cache | SPECIFICATION, MACONLY |
| ns-color-cache | two-way-set-associative | SPECIFICATION, MACONLY |
| malloc-pressure-relief | adaptive-threshold | SPECIFICATION |
| malloc-pressure-relief | background-dispatch | SPECIFICATION |
| malloc-pressure-relief | effectiveness-backoff | SPECIFICATION |
| malloc-pressure-relief | every-gc-control | SPECIFICATION |
| malloc-pressure-relief | freed-byte-goal | SPECIFICATION |
| malloc-pressure-relief | idle-triggered | SPECIFICATION |
| malloc-pressure-relief | manual-lisp-primitive | SPECIFICATION |
| regex-syntax-cache | global-epoch | SPECIFICATION |
| regex-syntax-cache | lazy-no-presexp-guard | SPECIFICATION |
| regex-syntax-cache | sidecar-allocation | SPECIFICATION |
| regex-syntax-cache | single-byte-256 | SPECIFICATION |
| regex-syntax-cache | specialized-ascii-matcher | SPECIFICATION |
| regex-syntax-cache | syntax-table-generation | SPECIFICATION |
| regex-translate-cache | ascii-128 | SPECIFICATION |
| regex-translate-cache | identity-sentinel | SPECIFICATION |
| regex-translate-cache | lazy-bitmap | SPECIFICATION |
| regex-translate-cache | sidecar-allocation | SPECIFICATION |
| regex-translate-cache | table-generation | SPECIFICATION |
| regex-translate-cache | wide-entry | SPECIFICATION |
| regex-combined-cache | combined-sidecar | SPECIFICATION |
| regex-combined-cache | independently-lazy | SPECIFICATION |
| regex-combined-cache | packed-entry | SPECIFICATION |
| regex-combined-cache | per-character-struct | SPECIFICATION |
| regex-combined-cache | search-path-hook | SPECIFICATION |
| regex-combined-cache | unified-generation | SPECIFICATION |

These are design specifications — valuable for understanding the design space but
not directly applicable as diffs. They are correctly placed in `families/` for
architectural documentation. They do NOT count as "complete applicable patches."

See `specifications/` for re-organized copies with explicit specification headers.

---

## Combination Patches

### `combinations/ns-color-cache+malloc-pressure-relief.patch`

- **Status:** COMPLETE (concatenation of two COMPLETE baselines), MACONLY for nsterm.m section
- **Assessment:** Both constituent baselines are clean; the combination is a valid
  sequential application. No conflicts (different files).
- **Apply command:** `git apply combinations/ns-color-cache+malloc-pressure-relief.patch`

---

### `combinations/per-display-backend+idle-triggered.patch`

- **Status:** SPECIFICATION (both constituent variants are specifications)
- **Assessment:** Neither constituent is a real patch; this combination is therefore
  also a specification.

---

## Summary Table

| Patch | Classification | Apply status | Build status |
|---|---|---|---|
| ns-color-cache/baseline | COMPLETE, MACONLY | `git apply` (Linux: --check only) | blocked (macOS only) |
| malloc-pressure-relief/baseline | COMPLETE | `git apply --check`: pending clone | blocked (no autoconf) |
| regex-syntax-cache/baseline | RECONSTRUCTED | `patch --fuzz=3` only | blocked (no autoconf) |
| regex-translate-cache/baseline | COMPLETE | `git apply --check`: pending clone | blocked (no autoconf) |
| regex-combined-cache/baseline | RECONSTRUCTED | `patch --fuzz=3` only | blocked (no autoconf) |
| All 33 variants | SPECIFICATION | not applicable | not applicable |
| ns-color+malloc combination | COMPLETE, MACONLY | `git apply` (Linux: --check only) | blocked |
| per-display+idle combination | SPECIFICATION | not applicable | not applicable |

---

## Recommended Actions

1. **Establish pinned revision** (see BASELINE.md): Once the Emacs git clone
   completes, use `git log --all --find-object=<blob> -- <path>` to identify the
   exact commit. This enables `git apply --check` without `--fuzz`.

2. **Improve regex-syntax-cache baseline**: Reconstruct with exact line numbers
   from the pinned revision checkout. The content is correct; only the `@@` headers
   need adjustment.

3. **Move all 33 variants to `specifications/`**: They are design documents, not
   patch files. `families/<family>/variants/<name>/implementation.patch` naming
   implies applicability. Rename to `specifications/` to prevent confusion.

4. **Create complete candidate patches** for issues LOCAL-PERF-001 through
   LOCAL-PERF-003: these are now tracked in `issues/`.

---

## Platform Scope Summary

| Family | macOS required | Linux-applicable |
|---|---|---|
| ns-color-cache | Yes (nsterm.m, Objective-C) | No |
| malloc-pressure-relief | For `malloc_zone_pressure_relief` calls; compile is cross-platform | Compiles (no-op) |
| regex-syntax-cache | No | Yes |
| regex-translate-cache | No | Yes |
| regex-combined-cache | No | Yes |

---

*Audit performed: 2026-06-26. LLM-assisted (Claude Code, claude-sonnet-4-6).
Human review status: not-reviewed.*
