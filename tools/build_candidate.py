#!/usr/bin/env python3
"""LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
Build Emacs from the patched worktree. Requires autoconf, libncurses-dev, etc.

Usage:
    python3 tools/build_candidate.py --patch families/.../implementation.patch
                                     [--emacs-dir /tmp/emacs-src]
                                     [--jobs N]
                                     [--configure-flags FLAG...]

This script:
1. Creates a temporary worktree at the pinned base commit
2. Applies the patch
3. Runs ./autogen.sh, ./configure, make -jN
4. Records output to issues/<issue>/build-logs/

Exit code: 0 on success, 1 on failure. Cleans up worktree on exit.
"""

import subprocess, sys, os, tempfile, shutil, argparse, datetime

BASE_COMMIT = "3ca168b80ae6d7b25fe55784dde3ad24faff7be2"
REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def run_step(name, args, cwd, log_fh):
    """Run a build step; write output to log_fh. Return exit code."""
    print(f"  Running: {' '.join(str(a) for a in args)}")
    log_fh.write(f"\n=== {name} ===\n")
    log_fh.write(f"$ {' '.join(str(a) for a in args)}\n")
    result = subprocess.run(
        args, cwd=cwd,
        stdout=log_fh, stderr=subprocess.STDOUT,
        text=True
    )
    log_fh.write(f"\nExit code: {result.returncode}\n")
    return result.returncode


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--patch", required=True)
    parser.add_argument("--emacs-dir", default="/tmp/emacs-src")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    parser.add_argument("--configure-flags", nargs="*", default=[
        "--without-x", "--without-ns", "--without-dbus",
        "--without-gconf", "--without-gsettings",
        "--without-makeinfo", "--with-gnutls=no"
    ])
    parser.add_argument("--log-dir", default=None,
                        help="Directory for build logs")
    args = parser.parse_args()

    patch_path = os.path.abspath(args.patch)
    if not os.path.isfile(patch_path):
        print(f"ERROR: patch not found: {patch_path}", file=sys.stderr)
        sys.exit(1)

    if not os.path.isdir(args.emacs_dir):
        print(f"ERROR: Emacs dir not found: {args.emacs_dir}", file=sys.stderr)
        sys.exit(1)

    log_dir = args.log_dir or tempfile.mkdtemp(prefix="emacs-build-log-")
    os.makedirs(log_dir, exist_ok=True)
    log_path = os.path.join(log_dir, "build.log")

    worktree = tempfile.mkdtemp(prefix="emacs-build-wt-")
    print(f"Build worktree: {worktree}")
    print(f"Log: {log_path}")

    try:
        with open(log_path, "w") as log:
            log.write(f"Build log: {datetime.datetime.utcnow().isoformat()}\n")
            log.write(f"Base commit: {BASE_COMMIT}\n")
            log.write(f"Patch: {patch_path}\n")
            log.write(f"Configure flags: {args.configure_flags}\n\n")

            # Create worktree
            print("Creating worktree...")
            rc = run_step("git worktree add",
                          ["git", "worktree", "add", worktree, BASE_COMMIT],
                          args.emacs_dir, log)
            if rc != 0:
                print("FAIL: git worktree add")
                return 1

            # Apply patch
            print("Applying patch...")
            rc = run_step("git apply",
                          ["git", "apply", "--verbose", patch_path],
                          worktree, log)
            if rc != 0:
                print("FAIL: git apply")
                return 1

            # autogen.sh
            print("Running autogen.sh...")
            rc = run_step("autogen.sh",
                          ["./autogen.sh"],
                          worktree, log)
            if rc != 0:
                print("FAIL: autogen.sh (is autoconf installed?)")
                return 1

            # configure
            print("Running configure...")
            configure_args = ["./configure"] + args.configure_flags
            rc = run_step("configure", configure_args, worktree, log)
            if rc != 0:
                print("FAIL: configure")
                return 1

            # make
            print(f"Running make -j{args.jobs}...")
            rc = run_step("make", ["make", f"-j{args.jobs}"], worktree, log)
            if rc != 0:
                print("FAIL: make")
                return 1

        print(f"\nSUCCESS: Build complete. Log: {log_path}")
        return 0

    finally:
        subprocess.run(
            ["git", "worktree", "remove", "--force", worktree],
            cwd=args.emacs_dir, capture_output=True
        )
        shutil.rmtree(worktree, ignore_errors=True)
        print(f"Worktree {worktree} cleaned up.")


if __name__ == "__main__":
    sys.exit(main())
