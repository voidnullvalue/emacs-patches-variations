# Benchmark Methodology
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
     Human review status: not-reviewed -->

This document describes the benchmark design, measurements taken, and interpretation.

---

## Standalone C Microbenchmarks

### Design Principles

1. **Deterministic loop:** Each benchmark runs exactly `ITERATIONS` (100 million)
   operations per run.
2. **Anti-dead-code-elimination:** Results are accumulated into a `volatile int`
   to prevent the compiler from optimizing away the benchmark loop.
3. **Warmup:** A configurable number of warmup runs precede measurement to
   prime caches and branch predictors.
4. **Multiple samples:** A configurable number of measured runs are collected;
   median is the primary statistic.
5. **No artificial interference:** No timing within the inner loop; only wall-clock
   time for the entire run.

### Simulated Operations

| Benchmark | Slow path simulation | Fast path simulation |
|---|---|---|
| bench_syntax_overhead | `noinline` function call + two pointer dereferences | Direct array access `bufp->syntax_ascii[c]` |
| bench_translate_overhead | `noinline` function call + one pointer dereference | Direct array access `bufp->trt_ascii[c]` |
| bench_combined_overhead | Both slow paths combined | Both array accesses combined |

The slow path uses `__attribute__((noinline))` to prevent the compiler from
inlining and eliminating the call overhead, giving a conservative simulation.
Real `syntax_property` and `char_table_translate` have additional overhead
(type dispatch, range checks, char-table traversal) not simulated here.

### Output Format

Each benchmark outputs:
- Per-run timings (seconds, nanoseconds/op)
- Summary table (median, mean, stdev)
- JSON block for machine-readable archival

### Stored Results

Raw benchmark output is stored in:
```
issues/<id>/benchmarks/results/raw.txt   — full output
issues/<id>/benchmarks/results/raw.json  — JSON block only
```

### Interpreting Results

**The microbenchmark speedup is for the isolated operation only.** In real Emacs:

- The syntax lookup path is only active in word-boundary regex opcodes
- The translate lookup path is active in case-fold isearch and regex match
- The fraction of time these paths consume depends heavily on workload

Conservative estimate for whole-system speedup in font-lock-heavy scenarios:
- If the measured path accounts for 10% of runtime: speedup ≈ 1.1× overall
- If the measured path accounts for 30% of runtime: speedup ≈ 1.5× overall
- Reported per-path speedups (2–3×) translate to whole-system speedup via Amdahl's law

Do not cite the microbenchmark numbers as whole-system speedup.

---

## Full Emacs Benchmark (not available on this host)

When an Emacs binary is available, the recommended benchmark is:

```sh
# Record time for font-lock and isearch on a large source file
emacs --batch -l /dev/null \
  --eval '(progn
    (find-file "/path/to/large-file.el")
    (emacs-lisp-mode)
    (font-lock-ensure)
    (let ((t0 (float-time)))
      (dotimes (_ 200)
        (save-excursion
          (goto-char (point-min))
          (while (re-search-forward "[[:word:]]+" nil t))))
      (message "%.3f s" (- (float-time) t0))))' 2>&1
```

This tests the exact hot path exercised by the syntax cache patch.

---

## Machine and Environment

```
OS:        Linux 6.12.41+deb13-amd64
CPU:       8 cores (see /proc/cpuinfo)
Compiler:  gcc (Debian 14.2.0-19) 14.2.0
Flags:     -O2
Clock:     CLOCK_MONOTONIC
```

All benchmark results in `issues/*/benchmarks/results/` were produced on this host.
