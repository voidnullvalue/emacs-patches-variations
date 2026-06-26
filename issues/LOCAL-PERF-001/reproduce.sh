#!/bin/sh
# LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
# Deterministic reproduction for LOCAL-PERF-001:
# Repeated syntax_property calls for ASCII chars in regex word-boundary opcodes.
#
# This script compiles and runs a standalone C microbenchmark that measures
# the overhead of simulated syntax_property calls vs flat array lookups.
# No Emacs build is required.
#
# Usage: sh issues/LOCAL-PERF-001/reproduce.sh [warmup_reps] [bench_reps]
#
# Platform: Linux, macOS (any host with GCC/clang)
# Exit code: 0 on success, 1 on compilation failure

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

WARMUP="${1:-5}"
BENCH="${2:-10}"
CC="${CC:-cc}"
BENCH_SRC="$SCRIPT_DIR/benchmarks/bench_syntax_overhead.c"
TEST_SRC="$SCRIPT_DIR/tests/test_regression.c"
OUT_BENCH="/tmp/lp001_bench_$$"
OUT_TEST="/tmp/lp001_test_$$"

echo "=== LOCAL-PERF-001 Reproduction ==="
echo "Host:    $(uname -sr)"
echo "CC:      $("$CC" --version 2>/dev/null | head -1)"
echo "Warmup:  $WARMUP"
echo "Bench:   $BENCH"
echo ""

# Step 1: Run regression test (verifies correctness of cache logic)
echo "--- Step 1: Regression test ---"
if "$CC" -O2 -o "$OUT_TEST" "$TEST_SRC" 2>&1; then
    "$OUT_TEST"
    echo "Regression test: PASSED"
else
    echo "Regression test: COMPILATION FAILED"
    exit 1
fi
rm -f "$OUT_TEST"
echo ""

# Step 2: Run performance benchmark
echo "--- Step 2: Performance benchmark ---"
if "$CC" -O2 -o "$OUT_BENCH" "$BENCH_SRC" \
   -DWARMUP_REPS="$WARMUP" -DBENCH_REPS="$BENCH" -lm 2>&1; then
    "$OUT_BENCH"
else
    echo "Benchmark: COMPILATION FAILED"
    exit 1
fi
rm -f "$OUT_BENCH"

echo ""
echo "=== Reproduction complete ==="
echo "See issues/LOCAL-PERF-001/benchmarks/results/raw.json for stored results."
