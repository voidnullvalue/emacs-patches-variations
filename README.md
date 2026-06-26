# Emacs Performance Patch Atlas
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 to 2026-06-26 -->

This is an explicitly LLM-assisted Emacs performance-patch and design-space research
archive. It documents concrete patches, tests, standalone C microbenchmarks, and
alternative designs for five local Emacs performance pathologies.

Under the policy publicly applied by GNU/Emacs maintainers to LLM-assisted
contributions, the patches here are not presented as suitable for upstream submission.
LLM provenance is deliberately disclosed rather than hidden.
See [PROVENANCE.md](PROVENANCE.md) for the full provenance statement.

> **Project status: complete public record.**
> This repository has made the policy point it was created to document.
> No upstream submissions, automated submissions, maintainer-directed
> campaign, or further outreach are planned. The project is not intended
> to interfere with, delay, or obstruct Emacs development.

---

## Quick Navigation

| Document | Purpose |
|---|---|
| [PROVENANCE.md](PROVENANCE.md) | Full LLM provenance disclosure |
| [PATCH_READINESS_AUDIT.md](PATCH_READINESS_AUDIT.md) | Per-patch applicability audit |
| [BASELINE.md](BASELINE.md) | Pinned base Emacs revision |
| [BUG_COVERAGE.md](BUG_COVERAGE.md) | Coverage of performance pathologies |
| [DESIGN_SPACE.md](DESIGN_SPACE.md) | Design axes and variant rationale |
| [manifest.json](manifest.json) | Machine-readable patch index |
| [docs/coverage-claims.md](docs/coverage-claims.md) | What may and may not be claimed |

---

## What This Repository Demonstrates

Substantially LLM-assisted work can produce:

- patches that apply to a pinned Emacs revision;
- focused tests and standalone C microbenchmarks;
- materially distinct implementation alternatives across a design space;
- auditable provenance records;
- a concrete example of a provenance policy that excludes technically substantive
  work independently of technical review.

This repository does **not** claim:

- that publication prevents independent work on the same problems;
- that similar future code must be derived from this repository;
- that all variants are production-ready;
- that microbenchmark speedups equal whole-Emacs speedups;
- that the repository broadly covers the Emacs bug backlog.

---

## Optimization Families

Each family has one applicable baseline patch and a set of design-specification
variants. All 33 variants carry `index 0000000000` markers and are not directly
applicable as diffs. See [DESIGN_SPACE.md](DESIGN_SPACE.md) and
[manifest.json](manifest.json) for the full variant list.

### 1. NSColor Cache (`families/ns-color-cache/`)

Caches `NSColor` objects keyed by packed pixel value in `+colorWithUnsignedLong:`.

**1 baseline (applicable patch) + 8 variants (specifications) = 9 implementations**  
macOS-only (`src/nsterm.m`); no macOS build or benchmark environment was available.

### 2. Malloc Pressure Relief (`families/malloc-pressure-relief/`)

Calls `malloc_zone_pressure_relief(NULL, 0)` after GC to reclaim fully-free pages.

**1 baseline (applicable patch) + 7 variants (specifications) = 8 implementations**  
macOS API; no macOS build or benchmark environment was available.

### 3. Regex ASCII Syntax Cache (`families/regex-syntax-cache/`)

Flat array caching syntax codes for ASCII characters, replacing `syntax_property()`
calls in word-boundary opcodes.

**1 baseline (applicable patch) + 6 variants (specifications) = 7 implementations**  
Cross-platform. Standalone C microbenchmarks included.

### 4. Regex ASCII Translate Cache (`families/regex-translate-cache/`)

Flat array caching translation results, replacing `char_table_translate()` calls in
case-fold match paths.

**1 baseline (applicable patch) + 6 variants (specifications) = 7 implementations**  
Cross-platform. Standalone C microbenchmarks included.

### 5. Regex Combined Cache (`families/regex-combined-cache/`)

Applies both regex caches in a single non-conflicting patch.

**1 baseline (applicable patch) + 6 variants (specifications) = 7 implementations**  
Cross-platform. Standalone C microbenchmarks included.

---

## Documented Performance-Pathology Coverage

No item reached full validation: no complete patched Emacs build was run and no
full Emacs test suite was executed. The identifiers below are local research
identifiers, not upstream Emacs Bug# reports. Microbenchmark results measure
isolated operations, not whole-program Emacs speedup.

| ID | Issue | Level | Platform | Microbenchmark |
|---|---|---|---|---|
| LOCAL-PERF-001 | `syntax_property` hot-path overhead in word-boundary opcodes | 4 | all | standalone C bench |
| LOCAL-PERF-002 | `char_table_translate` hot-path overhead in case-fold loops | 4 | all | standalone C bench |
| LOCAL-PERF-003 | Combined regex cache composition and cumulative overhead | 4 | all | standalone C bench |
| LOCAL-PERF-004 | NSColor per-call allocation in `+colorWithUnsignedLong:` | 3 | macOS | blocked — no macOS host |
| LOCAL-PERF-005 | RSS ratchet from malloc zone free-list after GC | 3 | macOS | not run — no macOS host |

Level 3 = regression-tested; level 4 = microbenchmarked; level 5 (full Emacs
validation) was not reached for any item. See [BUG_COVERAGE.md](BUG_COVERAGE.md).

---

## Applying Patches

All baseline patches apply to Emacs commit `3ca168b80ae6d7b25fe55784dde3ad24faff7be2`
(Emacs 31 development, 2026-06-23). See [BASELINE.md](BASELINE.md) and
[PATCH_READINESS_AUDIT.md](PATCH_READINESS_AUDIT.md) for per-patch detail including
which baselines are reconstructed and may require `--fuzz`.

```sh
git -C /path/to/emacs-git worktree add /tmp/emacs-base \
  3ca168b80ae6d7b25fe55784dde3ad24faff7be2
git -C /tmp/emacs-base apply families/regex-translate-cache/baseline/implementation.patch
```

---

## Inventory

| Artifact class | Count | Status |
|---|---:|---|
| Baseline implementations | 5 | Real diffs; `git apply --check` verified |
| Design variants | 33 | Specifications; not applicable diffs |
| Cross-family compositions | 2 | 1 applicable, 1 specification-only |
| **Total patch artifacts** | **40** | Mixed readiness |
| Documented local performance pathologies | 5 | Focused standalone evidence |
| Issues with standalone microbenchmarks | 3 | Isolated hot-path measurements |
| Fully validated patched Emacs builds | 0 | Not completed |

---

## Contributing

Active expansion is not planned. Pull requests may be considered only when they:

- contain materially distinct LLM-generated or substantially LLM-assisted work;
- disclose the LLM tool and model when known;
- preserve complete provenance;
- distinguish applicable patches from specifications;
- accurately report apply, build, test, and benchmark status;
- avoid cosmetic or duplicate variants.

---

## Final Scope Statement

This repository is retained as a public, timestamped, explicitly LLM-assisted
research artifact. It is sufficient for its intended purpose: demonstrating
that a categorical policy against LLM-assisted contributions can exclude
concrete patches, tests, benchmarks, and conventional alternative designs.

It is not a campaign against Emacs maintainers, not a claim over the underlying
ideas, and not an attempt to prevent or delay independent development. No
additional upstream-facing action is planned.
