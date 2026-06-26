#!/usr/bin/env python3
"""LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
Run standalone C benchmarks for one or more issues. Outputs JSON results.

Usage:
    python3 tools/run_benchmark.py [--issue LOCAL-PERF-001] [--warmup N] [--runs N]
    python3 tools/run_benchmark.py  # runs all benchmarks

Exit code: 0 on success, 1 on any error.
"""

import subprocess, sys, os, glob, tempfile, json, platform

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

ISSUE_BENCHMARKS = {
    "LOCAL-PERF-001": {
        "src": "issues/LOCAL-PERF-001/benchmarks/bench_syntax_overhead.c",
        "flags": ["-O2"],
        "extra_libs": ["-lm"],
        "platform": "all",
    },
    "LOCAL-PERF-002": {
        "src": "issues/LOCAL-PERF-002/benchmarks/bench_translate_overhead.c",
        "flags": ["-O2"],
        "extra_libs": ["-lm"],
        "platform": "all",
    },
    "LOCAL-PERF-003": {
        "src": "issues/LOCAL-PERF-003/benchmarks/bench_combined_overhead.c",
        "flags": ["-O2"],
        "extra_libs": ["-lm"],
        "platform": "all",
    },
}


def compile_bench(src, flags, extra_libs, tmpdir, warmup, runs):
    out = os.path.join(tmpdir, "bench_bin")
    cc = os.environ.get("CC", "cc")
    args = [cc] + flags + [
        f"-DWARMUP_REPS={warmup}",
        f"-DBENCH_REPS={runs}",
        "-o", out, src
    ] + (extra_libs or [])
    result = subprocess.run(args, capture_output=True, text=True)
    if result.returncode != 0:
        return None, result.stderr
    return out, None


def run_bench(binary):
    result = subprocess.run([binary], capture_output=True, text=True)
    if result.returncode != 0:
        return None, result.stderr
    return result.stdout, None


def extract_json(output):
    """Extract JSON block from benchmark output."""
    marker = "=== JSON ===\n"
    idx = output.find(marker)
    if idx < 0:
        return None
    return output[idx + len(marker):]


def main():
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--issue", nargs="*", default=None,
                        help="Issue ID(s) to benchmark (default: all)")
    parser.add_argument("--warmup", type=int, default=3)
    parser.add_argument("--runs", type=int, default=5)
    args = parser.parse_args()

    issues = args.issue or sorted(ISSUE_BENCHMARKS.keys())
    is_macos = platform.system() == "Darwin"

    total_pass = total_fail = total_skip = 0
    all_results = []

    with tempfile.TemporaryDirectory(prefix="emacs-bench-") as tmpdir:
        for issue_id in issues:
            if issue_id not in ISSUE_BENCHMARKS:
                print(f"SKIP: {issue_id} (no benchmark registered)")
                total_skip += 1
                continue

            bench = ISSUE_BENCHMARKS[issue_id]
            if bench["platform"] == "macos" and not is_macos:
                print(f"BLOCKED (macOS only): {issue_id}")
                total_skip += 1
                continue

            src = os.path.join(REPO_ROOT, bench["src"])
            print(f"\n--- {issue_id}: {bench['src']} ---")

            binary, err = compile_bench(src, bench["flags"],
                                        bench.get("extra_libs", []),
                                        tmpdir, args.warmup, args.runs)
            if not binary:
                print(f"  COMPILE FAILED: {err}")
                total_fail += 1
                continue

            output, err = run_bench(binary)
            if not output:
                print(f"  RUN FAILED: {err}")
                total_fail += 1
                continue

            json_str = extract_json(output)
            if json_str:
                try:
                    result = json.loads(json_str)
                    speedup = result.get("speedup_median", "?")
                    print(f"  PASS: speedup={speedup:.2f}x")
                    all_results.append(result)
                    total_pass += 1
                except json.JSONDecodeError as e:
                    print(f"  JSON PARSE ERROR: {e}")
                    total_fail += 1
            else:
                print("  PASS (no JSON output)")
                total_pass += 1

    print(f"\n=== Benchmark results: {total_pass} passed, {total_fail} failed, {total_skip} skipped ===")
    if all_results:
        print("\nSummary table:")
        print(f"{'Issue':<20} {'Speedup':<10}")
        print("-" * 30)
        for r in all_results:
            print(f"{r.get('issue','?'):<20} {r.get('speedup_median',0):.2f}x")

    return 0 if total_fail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
