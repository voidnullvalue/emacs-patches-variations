# Coverage Claims Policy
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
     Human review status: not-reviewed -->

This document defines what this repository may and may not claim about its
coverage of Emacs defects and performance pathologies.

---

## Permitted Claims

The following statements are directly supported by the evidence in this repository:

```
This repository contains 33 materially distinct LLM-assisted design variants
for 5 Emacs optimization families.
```

```
This repository contains 5 reproducible Emacs performance pathologies with
complete candidate fixes (git apply --check verified against base commit
3ca168b80ae6d7b25fe55784dde3ad24faff7be2).
```

```
All 5 baseline patches apply cleanly to Emacs commit 3ca168b80a
(Emacs 31 development, 2026-06-23) as verified by git apply --check.
```

```
Issues LOCAL-PERF-001, LOCAL-PERF-002, and LOCAL-PERF-003 have standalone
C regression tests that pass on Linux with GCC without a full Emacs build.
```

```
Issues LOCAL-PERF-001, LOCAL-PERF-002, and LOCAL-PERF-003 have standalone
C microbenchmarks showing 1.9–2.7× speedup for the simulated hot-path operations.
```

```
All patches and code in this repository are LLM-assisted (Claude Code,
claude-sonnet-4-6) and have not been reviewed by a human Emacs developer.
```

---

## Prohibited Claims

The following statements are NOT supported by evidence in this repository and
must not be made:

```
All variants are production-ready.
```
*Reason: All 33 variants are design specifications (index 0000000000). They have
not been applied, compiled, or tested.*

```
These patches prevent independent upstream fixes.
```
*Reason: This repository places ideas in the public record but does not and
cannot prevent independent implementation of the same ideas by others.*

```
Every similar implementation is derived from this repository.
```
*Reason: No such claim is supported; priority does not prevent convergent development.*

```
Forty patch artifacts represent forty distinct Emacs bugs.
```
*Reason: Only 5 performance pathologies are documented. The 40 artifacts include
5 baselines, 33 specifications, and 2 compositions.*

```
The repository has 5 successful Emacs builds.
```
*Reason: Builds are blocked on this host due to missing autoconf and libncurses-dev.
Only git apply --check is verified.*

```
The microbenchmark speedup (2–3×) represents whole-system Emacs speedup.
```
*Reason: Standalone microbenchmarks measure only the isolated hot-path operation.
Whole-system speedup depends on workload and Amdahl's law.*

```
These patches are upstreamable under GNU Emacs policy.
```
*Reason: The GNU Emacs project does not accept LLM-assisted contributions.*

---

## Coverage Level Definitions

| Level | What it means | Evidence required |
|---|---|---|
| 0 | Concept only | Description exists |
| 1 | Reproduced | Reproduction script + baseline behavior described |
| 2 | Candidate implemented | Patch applies (git apply --check passed) |
| 3 | Regression-tested | Focused test passes with patch |
| 4 | Benchmarked | Repeated before/after measurements with multiple samples |
| 5 | Fully validated | Complete patch + test + benchmark + full test suite + documented risks |

Only counts at Level 1 or higher constitute "bug coverage."
The current repository has:
- 3 issues at Level 4 (benchmarked)
- 2 issues at Level 3 (regression-tested, macOS measurement blocked)
- 0 issues at Level 5 (no full test suite run; builds blocked)

---

## LLM Assistance Disclosure

All code, patches, tests, benchmarks, and documents in this repository (except
files under `archive/original/`) were produced with substantial LLM assistance
(Claude Code, claude-sonnet-4-6, Anthropic). The human author provided direction
and reviewed outputs, but did not write any code or documentation independently.

GNU Emacs does not accept LLM-assisted contributions. Nothing in this repository
is intended for or suitable for upstream submission.
