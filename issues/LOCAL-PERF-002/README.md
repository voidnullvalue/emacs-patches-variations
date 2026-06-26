# LOCAL-PERF-002: Repeated char_table_translate Calls for ASCII in Regex Case-Folding Loops
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
     Human review status: not-reviewed -->

## 1. Precise Description

In `re_match_2_internal` and `re_search_2` (src/regex-emacs.c), the `TRANSLATE(d)`
macro expands to `char_table_translate(translate, d)` when a case-fold translate
table is active. This function is called on **every character examined** during:

- Fastmap computation (`re_search_2` scan loops)
- Character match opcodes (`exactn`, `anychar` with `\n` check)
- Exact character comparison in match loops

For ASCII characters (code < 256), the translation result is **constant for the
lifetime of the compiled pattern** — the translate table is set at `re_compile_pattern`
time and does not change during a search. Despite this, Emacs calls `char_table_translate`
on every character, traversing an Emacs char-table object (a vector-like structure
with property inheritance) on each call.

The hot paths affected include `isearch` with `case-fold-search` enabled —
one of the most frequently invoked code paths in interactive Emacs use.

## 2. Exact Affected Emacs Commit / Tag

- **Base commit:** `3ca168b80ae6d7b25fe55784dde3ad24faff7be2`
- **Commit message:** "Merge from origin/emacs-31" (2026-06-23)
- **Nearest tag:** `emacs-31.0.90` (350 commits earlier)
- **Blob hashes (verified):**
  - `src/regex-emacs.c`: `d8ed8a462a1a2a9cf5a02f169d1127fa2aa5753b`
  - `src/regex-emacs.h`: `bcfff662f7b65868ab6098338e8105ae31b5d021`

## 3. Deterministic Reproduction

```sh
sh issues/LOCAL-PERF-002/reproduce.sh
```

The script compiles and runs a standalone C microbenchmark measuring
`char_table_translate` call overhead vs flat array lookup.

## 4. Baseline Measurements / Failing Behavior

| Scenario | Operations | Time (median) | Per-op |
|---|---|---|---|
| Simulated char_table_translate (unpatched) | 100 000 000 | see raw.json | ~3–10 ns |
| Flat array lookup (patched) | 100 000 000 | see raw.json | ~0.5–1 ns |

Expected speedup: 5–10× for the translation step itself in the hot paths.

Run `sh reproduce.sh` to get baseline measurements on this host.

## 5. Candidate Fix

**Primary:** `families/regex-translate-cache/baseline/implementation.patch`

Adds:
- `#define TRANSLATE_TRU(C)` macro: uses `bufp->trt_ascii[C]` for `C < 128`
  when `trt_ascii_valid` is set; falls back to `TRANSLATE(C)` otherwise
- `unsigned char trt_ascii[256]` field in `re_pattern_buffer` struct (covers 0-255)
- `bool_bf trt_ascii_valid : 1` field in `re_pattern_buffer` struct
- Initialization block in `re_compile_pattern`: fills `trt_ascii[0..255]` from
  `char_table_translate` + `CHAR_TO_BYTE_SAFE` when translate table is active
- 11 call-site replacements: `RE_TRANSLATE(translate, buf_ch)` and `TRANSLATE(buf_ch)`
  → `TRANSLATE_TRU(buf_ch)` across fastmap scan and match loops

**Alternative (SPECIFICATION):**
`families/regex-translate-cache/variants/lazy-bitmap/implementation.patch`
— Lazy fill with per-entry validity bitmap. Not a real patch.

**Selection rationale:** The baseline eagerly fills the full 256-entry table once
at compile time. The 256-byte cost per pattern is negligible; the eager fill
avoids branches in the hot path. The `CHAR_TO_BYTE_SAFE` truncation is safe
for codes ≥ 128 that are identity or map within the byte range.

## 6. Regression Test

`issues/LOCAL-PERF-002/tests/test_regression.c`

```sh
cc -O2 -o /tmp/t2 issues/LOCAL-PERF-002/tests/test_regression.c && /tmp/t2
```

Verifies correctness of `TRANSLATE_TRU` for ASCII uppercase→lowercase folding.
The test would fail to compile on unpatched Emacs (fields `trt_ascii`,
`trt_ascii_valid` do not exist in the base struct).

## 7. Apply, Build, Test, Benchmark Status

| Step | Status | Notes |
|---|---|---|
| `git apply --check` | **passed** | Applies to base commit `3ca168b80a` |
| `git apply` | **passed** | Clean baseline with real blob SHAs |
| `./autogen.sh` | blocked | No autoconf on this host |
| `./configure` | blocked | No libncurses-dev on this host |
| `make -jN` | blocked | Blocked by configure |
| Unit test (standalone C) | **passed** | `test_regression.c` passes |
| Benchmark (standalone C) | **passed** | `bench_translate_overhead.c` runs |
| Full `make check` | not-run | Blocked by build |

## 8. Remaining Risks and Limitations

1. **Build not verified:** `git apply --check` passed; compilation blocked by missing
   build dependencies.

2. **Codes 128–255 truncation:** `CHAR_TO_BYTE_SAFE(ch)` returns -1 for char codes
   > 255, so the cache stores the original byte `i` for codes 128-255 that translate
   to out-of-range values. This is correct for unibyte buffers but may lose
   information if a 128-255 code maps to a non-ASCII multibyte character.

3. **Translate table identity:** The cache is invalidated only by pattern recompilation.
   If `translate` is mutated in-place after `re_compile_pattern` returns (non-standard),
   the cache will be stale. Emacs's normal case-fold table is not mutated in-place.

4. **LLM-assisted:** All patches and tests are LLM-generated.
