#!/bin/sh
# LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
# Reproduction for LOCAL-PERF-005: malloc zone RSS ratchet.
# PLATFORM: macOS required for malloc_zone_pressure_relief measurement.
# On Linux, only the threshold logic test runs.
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CC="${CC:-cc}"
OS="$(uname -s)"

echo "=== LOCAL-PERF-005 Reproduction: malloc zone RSS ratchet ==="
echo "Platform: $OS"
echo ""

echo "--- Step 1: Threshold logic test (pure C, platform-independent) ---"
OUT_TEST="/tmp/lp005_test_$$"
"$CC" -O2 -o "$OUT_TEST" "$SCRIPT_DIR/tests/test_threshold_logic.c"
"$OUT_TEST" && echo "Threshold logic test: PASSED"
rm -f "$OUT_TEST"
echo ""

if [ "$OS" != "Darwin" ]; then
    echo "--- Steps 2-5: BLOCKED (macOS only) ---"
    echo "malloc_zone_pressure_relief is a macOS-only API."
    echo ""
    echo "Required environment for macOS:"
    echo "  macOS 10.7+, Xcode command-line tools, autoconf"
    echo ""
    echo "Expected macOS commands:"
    echo "  1. Build unpatched Emacs:"
    echo "     ./autogen.sh && ./configure && make -j\$(sysctl -n hw.ncpu)"
    echo ""
    echo "  2. Run RSS workload (in new Emacs --batch):"
    echo "     emacs --batch --eval '(progn (dotimes (_ 10) (gc)) (message \"%d\" (buffer-size)))'"
    echo "     # Record RSS before/after with: vmmap <pid> | grep TOTAL"
    echo "     # Or: ps -o rss= -p <pid>"
    echo ""
    echo "  3. Apply patch and rebuild:"
    echo "     git apply families/malloc-pressure-relief/baseline/implementation.patch"
    echo "     make -j\$(sysctl -n hw.ncpu)"
    echo ""
    echo "  4. Re-run workload and compare RSS trajectories"
    exit 0
fi

echo "macOS detected. Running full reproduction..."
echo "(This requires autoconf, Xcode tools, and an Emacs source checkout)"
echo "Set EMACS_SRC to the Emacs source directory."
echo ""
echo "Skipping full build; providing commands only."
echo "See README.md for full instructions."
