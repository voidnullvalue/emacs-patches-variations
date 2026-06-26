# Testing Methodology
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
     Human review status: not-reviewed -->

This document describes how tests are run, what they cover, and what they do not cover.

---

## Standalone C Tests

Every issue has a `tests/test_regression.c` (or equivalent) that:

- Compiles with `cc -O2` (no Emacs headers, no special libraries)
- Runs in under 1 second
- Exits with code 0 on pass, 1 on fail
- Verifies the **logic** of the patch (cache correctness, struct fields present)
- Does NOT require an Emacs binary or macOS APIs

These tests serve as evidence that:
1. The struct additions exist (test would fail to compile on unpatched struct)
2. The macro logic (SYNTAX_TRU, TRANSLATE_TRU) gives correct results
3. Cache invalidation works correctly

### What standalone tests do NOT verify

- That the patch applies to the real Emacs source (use `git apply --check`)
- That the modified C files compile as part of Emacs (requires build environment)
- That the modified code produces correct Emacs behavior end-to-end
- That performance improves in a real Emacs workload (use benchmarks)
- That macOS APIs work correctly (requires macOS)

---

## Standalone C Benchmarks

Every issue has a `benchmarks/bench_*.c` that:

- Compiles with `cc -O2 -lm`
- Simulates the hot-path call pattern in pure C
- Measures time for N million iterations (unpatched path vs patched path)
- Outputs a JSON summary with samples, median, mean, stdev, speedup

### Benchmark validity

The standalone benchmarks measure the **underlying operation cost**, not whole-system
Emacs performance. The speedup reported is for the isolated operation:
- `bench_syntax_overhead.c`: simulated `syntax_property` call vs array lookup
- `bench_translate_overhead.c`: simulated `char_table_translate` call vs array lookup
- `bench_combined_overhead.c`: both operations together

Actual Emacs speedup depends on:
- Fraction of time spent in the measured hot path
- Instruction cache and branch predictor state in context
- Whether the cache validity guard is satisfied
- Buffer content (ASCII ratio, font-lock density)

Do not extrapolate from these microbenchmark results to precise whole-system speedup.

---

## Platform Coverage

| Test type | Linux | macOS |
|---|---|---|
| Standalone C tests (regex issues) | run | run |
| Standalone C tests (cache logic) | run (pure C) | run |
| Standalone C tests (threshold logic) | run | run |
| NSColor ObjC tests | blocked | run |
| RSS measurement | blocked | run |
| Full Emacs build | blocked (no autoconf) | possible |
| Emacs ERT test suite | blocked | possible |

All blockers on Linux are documented in issue STATUS files.

---

## git apply --check

Every baseline patch is verified with:
```sh
git -C /tmp/emacs-base apply --check families/<family>/baseline/implementation.patch
```

Against the exact pinned base commit `3ca168b80ae6d7b25fe55784dde3ad24faff7be2`.
This verifies the patch applies without conflict or offset error.

---

## Full Build Requirements (not available on this host)

```sh
./autogen.sh         # requires autoconf
./configure [flags]  # requires libncurses-dev and other deps
make -jN             # requires GCC and all linked libraries
```

If these were available, the recommended configure flags for a minimal test build are:
```sh
./configure --without-x --without-ns --without-dbus \
            --without-gconf --without-gsettings \
            --without-makeinfo --with-gnutls=no \
            --without-json
```

This produces a terminal-only Emacs binary suitable for `make check`.
