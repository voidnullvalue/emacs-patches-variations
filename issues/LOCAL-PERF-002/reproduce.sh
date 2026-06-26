#!/bin/sh
# LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
# Deterministic reproduction for LOCAL-PERF-002:
# Repeated char_table_translate calls for ASCII chars in regex case-folding loops.
#
# Platform: Linux, macOS (any host with GCC/clang and -lm)
# Exit code: 0 on success, 1 on compilation failure

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WARMUP="${1:-5}"
BENCH="${2:-10}"
CC="${CC:-cc}"

OUT_BENCH="/tmp/lp002_bench_$$"
OUT_TEST="/tmp/lp002_test_$$"

echo "=== LOCAL-PERF-002 Reproduction ==="
echo "Host: $(uname -sr)"
echo ""

echo "--- Step 1: Regression test ---"
"$CC" -O2 -o "$OUT_TEST" "$SCRIPT_DIR/tests/test_regression.c"
"$OUT_TEST" && echo "Regression test: PASSED"
rm -f "$OUT_TEST"

echo ""
echo "--- Step 2: Performance benchmark ---"
"$CC" -O2 -lm -DWARMUP_REPS="$WARMUP" -DBENCH_REPS="$BENCH" \
    -o "$OUT_BENCH" "$SCRIPT_DIR/benchmarks/bench_translate_overhead.c"
"$OUT_BENCH"
rm -f "$OUT_BENCH"
