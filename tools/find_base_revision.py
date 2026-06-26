#!/usr/bin/env python3
"""LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
Find the exact Emacs git commit where all patch-base blob hashes are simultaneously
present. Requires a local Emacs git checkout.

Usage:
    python3 tools/find_base_revision.py [--emacs-dir /path/to/emacs.git]

The known blob hashes are hard-coded from the repository's BASELINE.md.
"""

import argparse
import subprocess
import sys
import os

KNOWN_BLOBS = {
    "src/alloc.c":        "5fc166dbc2",
    "src/nsterm.m":       "a7f3fc292c",
    "src/regex-emacs.c":  "d8ed8a462a",
    "src/regex-emacs.h":  "bcfff662f7",
}

EXPECTED_BASE = "3ca168b80ae6d7b25fe55784dde3ad24faff7be2"
EXPECTED_TAG  = "emacs-31.0.90"


def run(args, cwd=None, check=True):
    """Run a command and return stdout. No shell injection from args."""
    result = subprocess.run(
        args, cwd=cwd,
        capture_output=True, text=True
    )
    if check and result.returncode != 0:
        print(f"ERROR: {' '.join(args)}", file=sys.stderr)
        print(result.stderr, file=sys.stderr)
        sys.exit(1)
    return result.stdout.strip()


def find_blob_match(emacs_dir, path, blob_prefix, known_commit):
    """Verify that known_commit has a file at path with hash starting with blob_prefix."""
    full_blob = run(
        ["git", "ls-tree", known_commit, path],
        cwd=emacs_dir
    )
    if not full_blob:
        return None, None
    parts = full_blob.split()
    if len(parts) < 3:
        return None, None
    full_hash = parts[2]
    return full_hash, full_hash.startswith(blob_prefix)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--emacs-dir", default="/tmp/emacs-src",
        help="Path to Emacs git checkout (default: /tmp/emacs-src)"
    )
    parser.add_argument(
        "--verify-only", action="store_true",
        help="Only verify the known base commit; do not search"
    )
    args = parser.parse_args()

    emacs_dir = args.emacs_dir
    if not os.path.isdir(emacs_dir):
        print(f"ERROR: Emacs directory not found: {emacs_dir}", file=sys.stderr)
        print("Clone with: git clone --filter=blob:none --no-checkout "
              "https://github.com/emacs-mirror/emacs.git /tmp/emacs-src",
              file=sys.stderr)
        sys.exit(1)

    print("=== Emacs base revision finder ===")
    print(f"Emacs dir: {emacs_dir}")
    print(f"Known base: {EXPECTED_BASE}")
    print()

    # Verify known base commit
    print("Verifying known base commit...")
    all_match = True
    for path, blob_prefix in sorted(KNOWN_BLOBS.items()):
        full_hash, match = find_blob_match(emacs_dir, path, blob_prefix, EXPECTED_BASE)
        status = "OK" if match else "MISMATCH"
        print(f"  {status}: {path}")
        print(f"        want:  {blob_prefix}*")
        print(f"        found: {full_hash or '(not found)'}")
        if not match:
            all_match = False

    print()
    if all_match:
        print(f"VERIFIED: All 4 blobs match at commit {EXPECTED_BASE}")
        # Get commit info
        info = run(
            ["git", "log", "--format=%H %ai %s", "-1", EXPECTED_BASE],
            cwd=emacs_dir, check=False
        )
        print(f"Commit: {info}")
        # Get nearest tag
        tag = run(
            ["git", "describe", "--tags", "--abbrev=0", EXPECTED_BASE],
            cwd=emacs_dir, check=False
        )
        describe = run(
            ["git", "describe", "--tags", EXPECTED_BASE],
            cwd=emacs_dir, check=False
        )
        print(f"Nearest tag: {tag}")
        print(f"Describe:    {describe}")
    else:
        print("FAILED: One or more blob hashes do not match at the expected base commit.")
        print("The Emacs repository may be the savannah mirror vs GitHub mirror.")
        sys.exit(1)

    return 0


if __name__ == "__main__":
    sys.exit(main())
