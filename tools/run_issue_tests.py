#!/usr/bin/env python3
"""LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
Compile and run standalone C tests for one or more issues.

Usage:
    python3 tools/run_issue_tests.py [issue-id...]
    python3 tools/run_issue_tests.py  # run all issues

Exit code: 0 if all tests pass, 1 if any fail.
No shell injection from metadata (all args are validated).
"""

import subprocess
import sys
import os
import glob
import tempfile

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Issue → test file mapping (platform-dependent)
ISSUE_TESTS = {
    "LOCAL-PERF-001": [
        {
            "src": "issues/LOCAL-PERF-001/tests/test_regression.c",
            "flags": ["-O2"],
            "platform": "all",
        }
    ],
    "LOCAL-PERF-002": [
        {
            "src": "issues/LOCAL-PERF-002/tests/test_regression.c",
            "flags": ["-O2"],
            "platform": "all",
        }
    ],
    "LOCAL-PERF-003": [
        {
            "src": "issues/LOCAL-PERF-003/tests/test_regression.c",
            "flags": ["-O2"],
            "platform": "all",
        }
    ],
    "LOCAL-PERF-004": [
        {
            "src": "issues/LOCAL-PERF-004/tests/test_cache_logic.c",
            "flags": ["-O2"],
            "platform": "all",
        }
    ],
    "LOCAL-PERF-005": [
        {
            "src": "issues/LOCAL-PERF-005/tests/test_threshold_logic.c",
            "flags": ["-O2"],
            "platform": "all",
        }
    ],
}


def compile_and_run(src_rel, flags, tmpdir):
    """Compile src and run it. Returns (passed, stdout, stderr)."""
    src = os.path.join(REPO_ROOT, src_rel)
    if not os.path.isfile(src):
        return None, "", f"Source not found: {src}"

    out = os.path.join(tmpdir, "test_bin")
    cc = os.environ.get("CC", "cc")

    # Compile — no shell injection: args are a list, not a string
    compile_args = [cc] + flags + ["-o", out, src]
    result = subprocess.run(compile_args, capture_output=True, text=True)
    if result.returncode != 0:
        return False, result.stdout, f"Compile failed:\n{result.stderr}"

    # Run
    run_result = subprocess.run([out], capture_output=True, text=True)
    passed = run_result.returncode == 0
    return passed, run_result.stdout, run_result.stderr


def main():
    args = sys.argv[1:] or sorted(ISSUE_TESTS.keys())

    import platform
    is_macos = platform.system() == "Darwin"

    total_pass = 0
    total_fail = 0
    total_skip = 0

    with tempfile.TemporaryDirectory(prefix="emacs-issue-tests-") as tmpdir:
        for issue_id in args:
            if issue_id not in ISSUE_TESTS:
                print(f"SKIP: {issue_id} (no tests registered)")
                total_skip += 1
                continue

            print(f"\n--- {issue_id} ---")
            for test in ISSUE_TESTS[issue_id]:
                plat = test.get("platform", "all")
                if plat == "macos" and not is_macos:
                    print(f"  BLOCKED (macOS only): {test['src']}")
                    total_skip += 1
                    continue

                passed, stdout, stderr = compile_and_run(
                    test["src"], test["flags"], tmpdir
                )
                if passed is None:
                    print(f"  SKIP: {test['src']}\n    {stderr}")
                    total_skip += 1
                elif passed:
                    print(f"  PASS: {test['src']}")
                    print(stdout.rstrip())
                    total_pass += 1
                else:
                    print(f"  FAIL: {test['src']}")
                    print(stdout.rstrip())
                    print(stderr.rstrip())
                    total_fail += 1

    print(f"\n=== Results: {total_pass} passed, {total_fail} failed, {total_skip} skipped ===")
    return 0 if total_fail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
