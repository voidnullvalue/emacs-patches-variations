#!/usr/bin/env bash
# LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25
# Audit the files touched by each patch in archive/original/ and report
# which files each patch modifies. Used to detect composition errors.
#
# Usage: ./tools/audit_patch_contents.sh

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ORIGINALS="$REPO_ROOT/archive/original"

echo "=== Patch contents audit ==="
echo ""

for patch in "$ORIGINALS"/*.patch; do
  name="$(basename "$patch")"
  echo "--- $name ---"
  grep '^diff --git' "$patch" | sed 's|diff --git a/||;s| b/.*||' | sort -u | \
    while read -r f; do
      printf "  %s\n" "$f"
    done
  echo ""
done

echo "=== LLM-assisted audit (Claude Code, 2026-06-25) ==="
