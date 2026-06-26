#!/usr/bin/env python3
"""LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
Apply a candidate patch to a temporary Emacs worktree at the pinned base revision.

Usage:
    python3 tools/apply_candidate.py --patch families/.../implementation.patch
                                     [--emacs-dir /tmp/emacs-src]
                                     [--worktree /tmp/emacs-apply-test]
                                     [--check-only]

The worktree is created at the pinned base commit. The patch is applied
(or checked with --check-only). The worktree is removed on exit.

Exit code: 0 on success, 1 on failure.
"""

import argparse
import subprocess
import sys
import os
import shutil
import tempfile

BASE_COMMIT = "3ca168b80ae6d7b25fe55784dde3ad24faff7be2"


def run(args, cwd=None, check=True, capture=True):
    """Run a command; no shell injection from args list."""
    result = subprocess.run(
        args, cwd=cwd,
        capture_output=capture, text=True
    )
    if check and result.returncode != 0:
        print(f"ERROR: {' '.join(str(a) for a in args)}", file=sys.stderr)
        if capture:
            print(result.stderr, file=sys.stderr)
        sys.exit(1)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--patch", required=True,
                        help="Path to the patch file to apply")
    parser.add_argument("--emacs-dir", default="/tmp/emacs-src",
                        help="Path to Emacs git clone (default: /tmp/emacs-src)")
    parser.add_argument("--worktree", default=None,
                        help="Path for temporary worktree (default: auto /tmp/...)")
    parser.add_argument("--check-only", action="store_true",
                        help="Only run git apply --check, do not apply")
    args = parser.parse_args()

    patch_path = os.path.abspath(args.patch)
    emacs_dir  = args.emacs_dir

    if not os.path.isfile(patch_path):
        print(f"ERROR: Patch not found: {patch_path}", file=sys.stderr)
        sys.exit(1)

    if not os.path.isdir(emacs_dir):
        print(f"ERROR: Emacs dir not found: {emacs_dir}", file=sys.stderr)
        print("Run: git clone --filter=blob:none https://github.com/emacs-mirror/emacs.git /tmp/emacs-src")
        sys.exit(1)

    worktree = args.worktree
    cleanup_worktree = False
    if worktree is None:
        worktree = tempfile.mkdtemp(prefix="emacs-apply-test-")
        cleanup_worktree = True

    print(f"=== apply_candidate.py ===")
    print(f"Patch:    {patch_path}")
    print(f"Base:     {BASE_COMMIT}")
    print(f"Worktree: {worktree}")
    print(f"Mode:     {'check-only' if args.check_only else 'apply'}")
    print()

    try:
        # Create worktree
        if not os.path.isdir(os.path.join(worktree, ".git")) and \
           not os.path.isfile(os.path.join(worktree, ".git")):
            print(f"Creating worktree at {BASE_COMMIT[:12]}...")
            run(["git", "worktree", "add", worktree, BASE_COMMIT],
                cwd=emacs_dir)
            print("Worktree created.")

        # Apply or check
        cmd = ["git", "apply"]
        if args.check_only:
            cmd.append("--check")
        cmd.extend(["--verbose", patch_path])

        print(f"Running: {' '.join(cmd)}")
        result = run(cmd, cwd=worktree, check=False)

        if result.returncode == 0:
            print(f"\nSUCCESS: {'Check' if args.check_only else 'Apply'} passed.")
            print(result.stdout)
            return 0
        else:
            print(f"\nFAILED: {'Check' if args.check_only else 'Apply'} failed.")
            print(result.stderr)
            return 1

    finally:
        if cleanup_worktree and os.path.exists(worktree):
            # Remove worktree from git index
            run(["git", "worktree", "remove", "--force", worktree],
                cwd=emacs_dir, check=False)
            shutil.rmtree(worktree, ignore_errors=True)
            print(f"\nWorktree {worktree} cleaned up.")


if __name__ == "__main__":
    sys.exit(main())
