# Audit Log
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25
     Updated: 2026-06-26 — exact base commit confirmed; combined-cache patch corrected -->

This document records every extraction, correction, and reconstruction decision
made when converting the original patch set into clean, family-isolated baselines.

---

## Base Emacs Revision

**Update 2026-06-26:** The exact base commit is now confirmed (see BASELINE.md).

**Confirmed base commit:** `3ca168b80ae6d7b25fe55784dde3ad24faff7be2`
- Date: 2026-06-23
- Message: "Merge from origin/emacs-31"
- Branch: Emacs master (Emacs 31 development, post emacs-31.0.90)
- Nearest tag: emacs-31.0.90 + 350 commits

**Correction to original estimate:** The earlier estimate of "Emacs 29.x or early 30"
was incorrect. The actual base is Emacs 31 development code from June 2026.

**All five baseline patches verified** with `git apply --check` against this commit.

---

**Original note (superseded):** The patches could not be attributed to a specific
Emacs commit with confidence.

**Known blob hashes (base sides of diffs):**

| File | Base blob hash |
|---|---|
| `src/alloc.c` | `5fc166dbc2` |
| `src/nsterm.m` | `a7f3fc292c` |
| `src/regex-emacs.c` | `d8ed8a462a` |
| `src/regex-emacs.h` | `bcfff662f7` |

**Contextual evidence that constrains the revision:**
- `alloc.c` references `malloc_probe`, `byte_ct`, and `total_bytes_of_live_objects()`,
  none of which existed before Emacs 28.
- `alloc.c` line 6012 is the GC profiling block; this line number is consistent
  with Emacs 29.x or the early 30.x development line.
- `regex-emacs.c` references `CHAR_TO_BYTE_SAFE`, which is present in Emacs 29+.
- `regex-emacs.h` uses `bool_bf` bitfield syntax that appears in Emacs 28+.

**Conclusion:** Emacs 29.x or early Emacs 30 pre-release, exact commit unknown.
The blob hashes above are the authoritative anchors; anyone with a checkout of
Emacs can run `git cat-file -e <hash>` to verify.

---

## Composition Errors Found

### Error 1 — `re_ascii_caches_0001.patch`

**Stated purpose (from original README):** "Adds an ASCII cache for
regular-expression symbol lookup."

**Actual contents:**
1. A complete `src/alloc.c` diff adding malloc-zone pressure relief — byte-for-byte
   identical to `memory_compact_0001.patch`. This is entirely unrelated to regex
   and should not be in this file.
2. A complete `src/regex-emacs.c` + `src/regex-emacs.h` diff adding **both** the
   syntax lookup cache (`SYNTAX_TRU`, `syntax_ascii`, `syntax_ascii_valid`) **and**
   the translate/case-fold cache (`TRANSLATE_TRU`, `trt_ascii`, `trt_ascii_valid`).
   The stated purpose implies only the syntax cache should be here.

**Evidence of the error:**
- The translate-only patch (`re_ascii_translate_cache_0001.patch`) applies to
  `regex-emacs.c` with base blob `d8ed8a462a`. The combined patch also starts from
  `d8ed8a462a`. Both must be independently applicable to the same base; the combined
  patch therefore erroneously merged two independent changes.
- Line-number offsets confirm the combination: translate-patch hunk
  `@@ -3503,7 +3519,7 @@`; the same hunk in `re_ascii_caches_0001.patch` is
  `@@ -3503,7 +3532,7 @@`. The +13 offset exactly equals the 13 lines added by the
  `SYNTAX_TRU` macro definition that precedes it in the file.

**Action taken:**
- `re_ascii_caches_0001.patch` preserved verbatim under `archive/original/`.
- `families/regex-syntax-cache/baseline/implementation.patch` reconstructed to
  contain only the syntax cache changes (see Reconstruction Note 1 below).

---

### Error 2 — `ascii_caches_combined_0001.patch`

**Stated purpose (from original README):** "Combines the two regex ASCII cache
patches (`re_ascii_caches_0001.patch` and `re_ascii_translate_cache_0001.patch`)."

**Actual contents:**
1. A complete `src/alloc.c` diff (malloc-zone pressure relief) — unrelated to regex.
2. A complete `src/nsterm.m` diff (NSColor cache) — unrelated to regex.
3. A `src/regex-emacs.c` + `src/regex-emacs.h` diff combining both regex caches.
   This part is correct for the stated purpose.

**Evidence of the error:**
- The alloc.c section is byte-for-byte identical to `memory_compact_0001.patch`.
- The nsterm.m section is byte-for-byte identical to `ns_color_cache_0001.patch`.
- Neither belongs in a file that "combines the two regex ASCII cache patches."

**Action taken:**
- `ascii_caches_combined_0001.patch` preserved verbatim under `archive/original/`.
- `families/regex-combined-cache/baseline/implementation.patch` reconstructed to
  contain only the `src/regex-emacs.{c,h}` sections (see Reconstruction Note 2 below).

---

## Reconstruction Notes

### Note 1 — `regex-syntax-cache` baseline

The syntax-only patch is reconstructed by taking `re_ascii_caches_0001.patch` and:
1. Removing the entire `src/alloc.c` diff block.
2. Retaining the `SYNTAX_TRU` macro definition hunk in `regex-emacs.c`.
3. Removing all `TRANSLATE_TRU` macro definition and usage hunks.
4. Retaining all `SYNTAX_TRU` usage hunks (those replacing `SYNTAX(c)` calls).
5. Splitting the final `re_compile_pattern` initialization hunk: retaining only the
   `syntax_ascii` population block; removing the `trt_ascii` population block.
6. In `regex-emacs.h`: retaining only the `syntax_ascii[128]` and
   `syntax_ascii_valid` field declarations; removing `trt_ascii[256]` and
   `trt_ascii_valid`.

**Line-number adjustment:** Because `TRANSLATE_TRU` (16 lines) is absent, all `+`
line numbers in hunks after line 1196 are reduced by 16 relative to the combined
patch. The reconstructed patch has `+` offsets that assume only the 13-line
`SYNTAX_TRU` macro is present when the syntax-usage and init hunks are applied.

**Status:** Line numbers are reconstructed approximations. Apply with
`patch -p1 --fuzz=3` or adjust line numbers after locating the context in the
actual source tree.

---

### Note 2 — `regex-combined-cache` baseline

The combined-regex patch is reconstructed by taking `ascii_caches_combined_0001.patch`
and removing:
1. The entire `src/alloc.c` diff block.
2. The entire `src/nsterm.m` diff block.

The `src/regex-emacs.c` and `src/regex-emacs.h` sections are extracted verbatim;
their blob hashes (`d8ed8a462a` base, `9932824d6c` target for regex-emacs.c;
`bcfff662f7` base, `b6f2510ed0` target for regex-emacs.h) are the correct hashes
for this content.

---

## Clean Patches — Summary

| Family | Baseline source | Reconstruction needed |
|---|---|---|
| `ns-color-cache` | `ns_color_cache_0001.patch` | No — already clean |
| `malloc-pressure-relief` | `memory_compact_0001.patch` | No — already clean |
| `regex-syntax-cache` | `re_ascii_caches_0001.patch` | Yes — see Note 1 |
| `regex-translate-cache` | `re_ascii_translate_cache_0001.patch` | No — already clean |
| `regex-combined-cache` | `ascii_caches_combined_0001.patch` | Yes — see Note 2 |

---

---

## Additional Correction: regex-combined-cache Baseline (2026-06-26)

The `families/regex-combined-cache/baseline/implementation.patch` (reconstructed
from `ascii_caches_combined_0001.patch`) had two formatting errors that prevented
clean application with `git apply`:

1. **Double bare blank lines:** After `bufp);` in the last hunk of `regex-emacs.c`,
   the patch had two consecutive bare newlines (no leading space). A unified diff
   context line must have a leading space; bare newlines are invalid hunk content.

2. **Wrong hunk line count:** The `@@ -5348,6 +5377,41 @@` header claimed 41 new
   lines, but the actual hunk content had 40 lines (6 context + 34 added).

**Fix applied (2026-06-26):** The patch was regenerated from a clean combined
state by:
1. Applying `regex-translate-cache/baseline/implementation.patch` to the base
2. Manually adding `SYNTAX_TRU` macro, struct fields, opcode replacements, and
   initialization block
3. Generating a fresh `git diff` from the combined state
4. Verifying with `git apply --check` (exit 0, confirmed)

The semantics are identical to the original intent; only the formatting was fixed.

---

## Semantic Review

No semantic alteration was made to any baseline. Reconstructed patches contain
identical code to the relevant sections of their source files. The only change is
exclusion of code that did not belong, or correction of formatting errors that
prevented clean application.
