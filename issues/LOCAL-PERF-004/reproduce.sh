#!/bin/sh
# LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
# Reproduction for LOCAL-PERF-004: NSColor per-call allocation pathology.
#
# PLATFORM: macOS only. On Linux, only the pure-C cache-logic test runs.
#
# Usage: sh issues/LOCAL-PERF-004/reproduce.sh

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CC="${CC:-cc}"
OS="$(uname -s)"

echo "=== LOCAL-PERF-004 Reproduction: NSColor allocation overhead ==="
echo "Platform: $OS"
echo ""

# Step 1: Pure-C cache logic test (runs on Linux and macOS)
echo "--- Step 1: Cache logic test (pure C, platform-independent) ---"
OUT_TEST="/tmp/lp004_test_$$"
if "$CC" -O2 -o "$OUT_TEST" "$SCRIPT_DIR/tests/test_cache_logic.c"; then
    "$OUT_TEST" && echo "Cache logic test: PASSED"
else
    echo "Cache logic test: COMPILATION FAILED"
    exit 1
fi
rm -f "$OUT_TEST"

echo ""

if [ "$OS" != "Darwin" ]; then
    echo "--- Steps 2-5: BLOCKED (macOS only) ---"
    echo "The following steps require macOS, Xcode tools, and a full Emacs build:"
    echo "  2. Build unpatched Emacs --with-ns"
    echo "  3. Run frame-render benchmark (record NSColor allocation count)"
    echo "  4. Apply families/ns-color-cache/baseline/implementation.patch"
    echo "  5. Rebuild and re-run benchmark"
    echo ""
    echo "Expected result on macOS:"
    echo "  - Unpatched: +colorWithUnsignedLong: called N times per redisplay"
    echo "  - Patched:   +colorWithUnsignedLong: called O(faces) times total (cache hits)"
    exit 0
fi

# macOS path
echo "--- Step 2-5: macOS benchmark (requires autoconf, Xcode) ---"
EMACS_SRC="${EMACS_SRC:-/tmp/emacs-ns-test}"
EMACS_BIN_UNPATCHED="${EMACS_SRC}/emacs-unpatched"
EMACS_BIN_PATCHED="${EMACS_SRC}/emacs-patched"
PATCH="$SCRIPT_DIR/../../families/ns-color-cache/baseline/implementation.patch"

echo "NOTE: This step requires a full Emacs build environment."
echo "Set EMACS_SRC to an Emacs source checkout with NS support."
echo "Commands would be:"
echo "  cd \$EMACS_SRC && ./autogen.sh && ./configure --with-ns"
echo "  make -j\$(sysctl -n hw.ncpu)"
echo "  cp src/emacs \$EMACS_BIN_UNPATCHED"
echo "  git apply $PATCH"
echo "  make -j\$(sysctl -n hw.ncpu)"
echo "  cp src/emacs \$EMACS_BIN_PATCHED"
echo "  # Then benchmark both binaries using the bench script"
echo ""
echo "Benchmark Elisp (run with --batch):"
cat << 'ELISP'
(progn
  (require 'font-lock)
  (let* ((start (float-time))
         (buf (find-file-noselect "/path/to/large-source-file.el")))
    (with-current-buffer buf
      (emacs-lisp-mode)
      (font-lock-ensure)
      (dotimes (_ 100)
        (redisplay t)))
    (message "Redisplay time: %.3fs" (- (float-time) start))))
ELISP
