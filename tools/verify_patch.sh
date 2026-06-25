#!/usr/bin/env bash
# LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25
# Verify that a patch applies cleanly to a given Emacs source tree.
# Prints apply status for every implementation.patch in the repository.
#
# Usage: ./tools/verify_patch.sh <emacs-source-dir>
#
# Exit code: 0 if all patches apply (possibly with fuzz), non-zero otherwise.

set -euo pipefail

EMACS_SRC="${1:-}"
if [[ -z "$EMACS_SRC" || ! -d "$EMACS_SRC" ]]; then
  echo "Usage: $0 <emacs-source-dir>" >&2
  exit 1
fi

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PATCH_CMD="patch"
PASS=0
FAIL=0
SKIP=0

check_patch() {
  local patch_file="$1"
  local rel="${patch_file#$REPO_ROOT/}"

  # Skip archive originals
  if [[ "$patch_file" == *"/archive/original/"* ]]; then
    return
  fi

  # Skip if patch contains placeholder index hashes (design-space variants)
  if grep -q 'index.*0000000000' "$patch_file" 2>/dev/null; then
    echo "SKIP (design-space): $rel"
    ((SKIP++)) || true
    return
  fi

  # Dry-run with fuzz=3
  if "$PATCH_CMD" -p1 --fuzz=3 --dry-run \
       --directory="$EMACS_SRC" \
       < "$patch_file" >/dev/null 2>&1; then
    echo "PASS: $rel"
    ((PASS++)) || true
  else
    echo "FAIL: $rel"
    ((FAIL++)) || true
  fi
}

echo "=== Patch verification against: $EMACS_SRC ==="
echo ""

while IFS= read -r -d '' patch; do
  check_patch "$patch"
done < <(find "$REPO_ROOT" -name "implementation.patch" -print0 | sort -z)

echo ""
echo "Results: PASS=$PASS  FAIL=$FAIL  SKIP=$SKIP"

[[ "$FAIL" -eq 0 ]]
