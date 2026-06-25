#!/usr/bin/env bash
# LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25
# Benchmark driver for regex cache patches.
# Measures wall-clock time for font-lock and isearch operations
# before/after applying a patch.
#
# Usage: ./bench_regex_caches.sh [emacs_binary] [warmup_reps] [bench_reps]
#
# Requires: emacs binary, time(1), a sample large Emacs Lisp source file.

set -euo pipefail

EMACS="${1:-emacs}"
WARMUP="${2:-3}"
BENCH="${3:-10}"
SAMPLE_FILE="${4:-$(ls /usr/share/emacs/*/lisp/emacs-lisp/bytecomp.el.gz 2>/dev/null | head -1)}"

if [[ -z "$SAMPLE_FILE" ]]; then
  echo "Error: no sample file found. Pass a .el file as argument 4." >&2
  exit 1
fi

echo "=== regex-caches benchmark ==="
echo "Emacs:       $EMACS"
echo "Sample file: $SAMPLE_FILE"
echo "Warmup reps: $WARMUP"
echo "Bench reps:  $BENCH"
echo ""

# Elisp to run: open file, font-lock it, time isearch across it.
BENCH_ELISP='
(progn
  (setq inhibit-message t)
  (find-file-noselect (car command-line-args-left))
  (with-current-buffer (car (buffer-list))
    (emacs-lisp-mode)
    (font-lock-ensure)
    (let ((start (float-time)))
      (dotimes (_ 100)
        (save-excursion
          (goto-char (point-min))
          (while (re-search-forward "[[:word:]]+" nil t))))
      (message "isearch-loop: %.3fs" (- (float-time) start)))))
'

run_bench() {
  local label="$1"
  local total=0
  for _ in $(seq 1 "$WARMUP"); do
    "$EMACS" --batch -l /dev/null \
      --eval "$BENCH_ELISP" "$SAMPLE_FILE" 2>/dev/null || true
  done
  for _ in $(seq 1 "$BENCH"); do
    t=$( { time "$EMACS" --batch -l /dev/null \
               --eval "$BENCH_ELISP" "$SAMPLE_FILE" 2>/dev/null; } \
         2>&1 | awk '/real/ {print $2}' | \
         sed 's/m/\*60+/;s/s//' | bc )
    total=$(echo "$total + $t" | bc)
  done
  avg=$(echo "scale=3; $total / $BENCH" | bc)
  echo "$label: avg ${avg}s over ${BENCH} runs"
}

run_bench "current-build"
echo ""
echo "To compare patches:"
echo "  1. Apply patch with: patch -p1 < families/<family>/baseline/implementation.patch"
echo "  2. Rebuild Emacs: make -C <emacs-src> -j\$(nproc)"
echo "  3. Run this script again pointing to the new binary"
echo ""
echo "=== LLM-assisted benchmark driver (Claude Code, 2026-06-25) ==="
