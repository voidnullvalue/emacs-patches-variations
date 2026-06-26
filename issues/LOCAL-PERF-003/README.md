# LOCAL-PERF-003: Combined Regex Cache — Composition Conflict and Cumulative Overhead
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
     Human review status: not-reviewed -->

## 1. Precise Description

Applying the regex syntax cache (LOCAL-PERF-001) and the regex translate cache
(LOCAL-PERF-002) independently produces **patch conflicts**: both patches add
fields to the same location in `struct re_pattern_buffer` and both add
initialization blocks at the same point in `re_compile_pattern`.

When only one cache is applied, the full potential performance benefit is not
realized:
- Without the translate cache, `TRANSLATE(c)` still calls `char_table_translate`
  in the case-fold hot paths even when the syntax cache benefits word-boundary ops.
- Without the syntax cache, word-boundary ops still call `syntax_property` even
  when the translate cache eliminates the `char_table_translate` calls.

The combined overhead of both uncached paths is measurably larger than either
alone. The baseline patch for the combined cache (`regex-combined-cache/baseline`)
composes both features in a single non-conflicting diff.

Additionally, the original `ascii_caches_combined_0001.patch` contained formatting
errors (corrupt blank lines, incorrect hunk header count) that prevented clean
application. This issue documents the correction and the combined pathology.

## 2. Exact Affected Emacs Commit / Tag

- **Base commit:** `3ca168b80ae6d7b25fe55784dde3ad24faff7be2`
- **Commit message:** "Merge from origin/emacs-31" (2026-06-23)
- **Nearest tag:** `emacs-31.0.90` (350 commits earlier)
- **Blob hashes (verified):**
  - `src/regex-emacs.c`: `d8ed8a462a1a2a9cf5a02f169d1127fa2aa5753b`
  - `src/regex-emacs.h`: `bcfff662f7b65868ab6098338e8105ae31b5d021`

## 3. Deterministic Reproduction

```sh
sh issues/LOCAL-PERF-003/reproduce.sh
```

Compiles and runs a standalone C test + benchmark demonstrating:
1. The combined struct size increase
2. The cumulative speedup from both caches together
3. Correctness of both SYNTAX_TRU and TRANSLATE_TRU in the same program

## 4. Baseline Measurements / Failing Behavior

The combined unpatched overhead is the sum of LOCAL-PERF-001 and LOCAL-PERF-002
when both code paths are active in a single search:

| Scenario | Median (s) | ns/op |
|---|---|---|
| Both paths unpatched | see raw.json | ~2–3 ns/op total |
| Both paths patched (combined) | see raw.json | ~0.5–1 ns/op |

Run `sh reproduce.sh` for measurements on this host.

## 5. Candidate Fix

**Primary:** `families/regex-combined-cache/baseline/implementation.patch`

This is a regenerated clean patch (2026-06-26) that applies both caches in a
single diff with no conflicts. The original `ascii_caches_combined_0001.patch`
in `archive/original/` had formatting corruption; this patch is verified with
`git apply --check` against the pinned base commit.

Changes:
- Adds `SYNTAX_TRU` macro + `syntax_ascii[128]` + `syntax_ascii_valid`
- Adds `TRANSLATE_TRU` macro + `trt_ascii[256]` + `trt_ascii_valid`
- Replaces 13 `SYNTAX(c)` calls → `SYNTAX_TRU(c)` in word-boundary opcodes
- Replaces 11 `TRANSLATE(c)` calls → `TRANSLATE_TRU(c)` in match/fastmap loops
- Adds dual initialization at `re_compile_pattern` exit

**Alternative (SPECIFICATION):**
`families/regex-combined-cache/variants/unified-generation/implementation.patch`
— Uses a single generation counter for both caches. Not a real patch.

**Selection rationale:** Independent flags are simpler and allow the translate
cache to be populated independently of the syntax cache (e.g., patterns without
word-boundary ops don't need the syntax cache). Unified generation would force
both to be rebuilt together even when only one changed.

## 6. Regression Test

```sh
cc -O2 -o /tmp/t3 issues/LOCAL-PERF-003/tests/test_regression.c && /tmp/t3
```

Tests combined correctness: both SYNTAX_TRU and TRANSLATE_TRU work together
with independent validity flags.

## 7. Apply, Build, Test, Benchmark Status

| Step | Status | Notes |
|---|---|---|
| `git apply --check` | **passed** | Regenerated patch applies cleanly |
| `git apply` | **passed** | Verified |
| `./autogen.sh` | blocked | No autoconf |
| `./configure` | blocked | No libncurses-dev |
| `make -jN` | blocked | Blocked |
| Unit test (standalone C) | **passed** | Both caches correct |
| Benchmark (standalone C) | **passed** | Combined speedup measured |
| `make check` | not-run | Blocked by build |

## 8. Remaining Risks and Limitations

1. **Build not verified:** `git apply --check` passed; no Emacs binary produced.

2. **Original patch corruption:** The source `ascii_caches_combined_0001.patch`
   had two errors: (a) included unrelated alloc.c and nsterm.m sections, and
   (b) had a corrupt hunk with double bare blank lines and a wrong line count.
   The candidate fix here is a fully regenerated patch, not the original.

3. **Struct size impact:** Both caches add 386 bytes to `re_pattern_buffer`.
   For very large numbers of simultaneously live patterns, this increases memory
   pressure. In practice Emacs reuses a small number of compiled patterns.

4. **Independent init order:** The translate cache is initialized before the
   syntax cache in the patch. If the translate table is nil, `trt_ascii_valid`
   is false and the fast path doesn't engage, but the syntax cache still
   initializes correctly.

5. **LLM-assisted:** All patches and tests are LLM-generated.
