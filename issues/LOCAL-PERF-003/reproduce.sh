#!/bin/sh
# LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
# Reproduction for LOCAL-PERF-003: combined regex caches.
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WARMUP="${1:-5}" BENCH="${2:-10}" CC="${CC:-cc}"
OUT_TEST="/tmp/lp003_test_$$" OUT_BENCH="/tmp/lp003_bench_$$"
echo "=== LOCAL-PERF-003 Reproduction ==="
echo "--- Step 1: Regression test ---"
"$CC" -O2 -o "$OUT_TEST" "$SCRIPT_DIR/tests/test_regression.c"
"$OUT_TEST" && echo "Regression test: PASSED"
rm -f "$OUT_TEST"
echo ""
echo "--- Step 2: Benchmark ---"
"$CC" -O2 -lm -DWARMUP_REPS="$WARMUP" -DBENCH_REPS="$BENCH" \
    -o "$OUT_BENCH" "$SCRIPT_DIR/benchmarks/bench_combined_overhead.c"
"$OUT_BENCH"
rm -f "$OUT_BENCH"
