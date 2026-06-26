#!/usr/bin/env python3
"""LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
Run the reproduction script for one or more issues.

Usage:
    python3 tools/run_reproduction.py [issue-id...]
    python3 tools/run_reproduction.py  # all issues

Exit code: 0 if all pass, 1 if any fail.
"""

import subprocess, sys, os, glob

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main():
    args = sys.argv[1:]
    if not args:
        args = sorted(d for d in os.listdir(os.path.join(REPO_ROOT, "issues"))
                      if os.path.isdir(os.path.join(REPO_ROOT, "issues", d)))

    total_pass = total_fail = 0
    for issue_id in args:
        script = os.path.join(REPO_ROOT, "issues", issue_id, "reproduce.sh")
        if not os.path.isfile(script):
            print(f"SKIP: {issue_id} (no reproduce.sh)")
            continue

        print(f"\n=== {issue_id}: running reproduce.sh ===")
        result = subprocess.run(
            ["sh", script],
            cwd=os.path.join(REPO_ROOT, "issues", issue_id)
        )
        if result.returncode == 0:
            print(f"PASS: {issue_id}")
            total_pass += 1
        else:
            print(f"FAIL: {issue_id} (exit {result.returncode})")
            total_fail += 1

    print(f"\n=== {total_pass} passed, {total_fail} failed ===")
    return 0 if total_fail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
