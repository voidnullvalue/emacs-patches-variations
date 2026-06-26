#!/usr/bin/env python3
"""LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
Validate issue metadata.yaml files against the required schema.

Usage:
    python3 tools/validate_issue_metadata.py [issue-dir...]
    python3 tools/validate_issue_metadata.py  # validates all issues/*/

Exit code: 0 if all valid, 1 if any errors found.
"""

import sys
import os
import glob

try:
    import yaml
except ImportError:
    print("ERROR: PyYAML not installed. pip install pyyaml", file=sys.stderr)
    sys.exit(2)

REQUIRED_FIELDS = [
    "id", "title", "classification", "source", "upstream_reference",
    "affected_revision", "nearest_release", "platforms", "files_involved",
    "reproduction_command", "baseline_status", "candidate_fixes",
    "regression_test", "benchmark", "llm_assisted", "llm_system",
    "llm_model", "human_review_status", "apply_status", "build_status",
    "test_status", "benchmark_status", "known_limitations",
]

VALID_CLASSIFICATIONS = {"bug", "regression", "performance-pathology"}
VALID_SOURCES = {"upstream-bug", "mailing-list", "source-analysis", "local-benchmark"}
VALID_STATUSES = {"passed", "failed", "blocked", "not-run"}
VALID_REVIEWS = {"not-reviewed", "reviewed", "approved"}


def validate_file(path):
    errors = []
    try:
        with open(path) as f:
            data = yaml.safe_load(f)
    except Exception as e:
        return [f"YAML parse error: {e}"]

    if not isinstance(data, dict):
        return ["Root element is not a mapping"]

    for field in REQUIRED_FIELDS:
        if field not in data:
            errors.append(f"Missing required field: {field}")

    if "classification" in data:
        if data["classification"] not in VALID_CLASSIFICATIONS:
            errors.append(
                f"Invalid classification: {data['classification']!r}; "
                f"must be one of {sorted(VALID_CLASSIFICATIONS)}"
            )

    if "source" in data:
        if data["source"] not in VALID_SOURCES:
            errors.append(
                f"Invalid source: {data['source']!r}; "
                f"must be one of {sorted(VALID_SOURCES)}"
            )

    for status_field in ["apply_status", "build_status", "test_status",
                          "benchmark_status", "baseline_status"]:
        val = data.get(status_field)
        if val is not None and val not in VALID_STATUSES:
            errors.append(
                f"Invalid {status_field}: {val!r}; "
                f"must be one of {sorted(VALID_STATUSES)}"
            )

    if data.get("human_review_status") not in VALID_REVIEWS:
        errors.append(f"Invalid human_review_status: {data.get('human_review_status')!r}")

    if not data.get("llm_assisted", False):
        errors.append("llm_assisted must be true")

    return errors


def main():
    args = sys.argv[1:]
    if not args:
        base = os.path.join(os.path.dirname(__file__), "..", "issues")
        args = sorted(glob.glob(os.path.join(base, "*/metadata.yaml")))

    if not args:
        print("No metadata.yaml files found.", file=sys.stderr)
        sys.exit(1)

    total_errors = 0
    for path in args:
        if os.path.isdir(path):
            path = os.path.join(path, "metadata.yaml")
        if not os.path.isfile(path):
            print(f"SKIP (not found): {path}")
            continue

        errors = validate_file(path)
        if errors:
            print(f"FAIL: {path}")
            for e in errors:
                print(f"  - {e}")
            total_errors += len(errors)
        else:
            issue_dir = os.path.basename(os.path.dirname(path))
            print(f"OK:   {issue_dir}")

    print()
    if total_errors:
        print(f"{total_errors} error(s) found.")
        return 1
    else:
        print(f"All {len(args)} metadata file(s) valid.")
        return 0


if __name__ == "__main__":
    sys.exit(main())
