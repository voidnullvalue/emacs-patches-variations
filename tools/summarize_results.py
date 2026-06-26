#!/usr/bin/env python3
"""LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
Summarize benchmark results across all issues.

Usage:
    python3 tools/summarize_results.py [--format text|json]
"""

import sys, os, glob, json

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def load_results():
    results = []
    for path in sorted(glob.glob(
            os.path.join(REPO_ROOT, "issues/*/benchmarks/results/raw.json"))):
        with open(path) as f:
            try:
                data = json.load(f)
                results.append(data)
            except json.JSONDecodeError:
                pass
    return results


def main():
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--format", choices=["text", "json"], default="text")
    args = parser.parse_args()

    results = load_results()
    if not results:
        print("No results found.", file=sys.stderr)
        return 1

    if args.format == "json":
        json.dump(results, sys.stdout, indent=2)
        print()
        return 0

    print("=== Benchmark Results Summary ===")
    print(f"{'Issue':<20} {'Patch':<50} {'Slow (med, s)':<15} {'Fast (med, s)':<15} {'Speedup'}")
    print("-" * 110)
    for r in results:
        issue   = r.get("issue", "?")
        patch   = os.path.basename(r.get("patch", "?"))
        slow    = r.get("slow_path", {}).get("median", 0)
        fast    = r.get("fast_path", {}).get("median", 0)
        speedup = r.get("speedup_median", 0)
        print(f"{issue:<20} {patch:<50} {slow:<15.4f} {fast:<15.4f} {speedup:.2f}x")
    return 0


if __name__ == "__main__":
    sys.exit(main())
