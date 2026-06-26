# Emacs Performance Patch Atlas
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 to 2026-06-26 -->

This repository is a **defensive publication and reproducible performance research
atlas** for a set of Emacs performance optimizations and their design-space variants.

**Every patch, document, test, benchmark, and tool in this repository was produced
with substantial LLM assistance (Claude Code, claude-sonnet-4-6).** See
[PROVENANCE.md](PROVENANCE.md) for the full provenance statement.

**GNU Emacs does not accept LLM-assisted contributions.** Nothing here is intended
for or suitable for upstream submission. See upstream policy in PROVENANCE.md.

---
This repository is retained as a public, timestamped, explicitly LLM-assisted research artifact. It is sufficient for its intended purpose: demonstrating that a categorical policy against LLM-assisted contributions can exclude concrete patches, tests, benchmarks, and conventional alternative designs.

It is not a campaign against Emacs maintainers, not a claim over the underlying ideas, and not an attempt to prevent or delay independent development. No additional upstream-facing action is planned.
::: 



## Important Disclosures

- **All content is LLM-assisted.** No patch or document was written by a human author
  independently. Human involvement was limited to direction and final decisions.
- **These patches are not upstreamable** under current GNU Emacs policy, which as
  publicly applied to LLM-assisted contributions, does not accept them.
- **Specifications vs. verified fixes:** 33 of the 38 patch artifacts are design
  specifications (marked `index 0000000000` in the diff header). Only the 5 baseline
  patches are real applicable diffs; see PATCH_READINESS_AUDIT.md.
- **Builds not verified on this host:** Build is blocked by missing autoconf and
  libncurses-dev. All git apply --check passes are verified; actual Emacs compilation
  is not.
- **Bug coverage is reported separately** from raw patch counts.
  See [BUG_COVERAGE.md](BUG_COVERAGE.md).
- **Pull requests are welcome** for additional materially distinct, LLM-generated or
  substantially LLM-assisted patches. All contributions must disclose LLM provenance.
  Cosmetic variants and duplicate diffs are rejected.
- **This repository does not claim ownership** over any ideas or prevent independent
  implementation of the same optimizations by others.

---

## Quick Navigation

| Document | Purpose |
|---|---|
| [PROVENANCE.md](PROVENANCE.md) | Full LLM provenance disclosure |
| [AUDIT.md](AUDIT.md) | Audit log: errors found, reconstructions made |
| [PATCH_READINESS_AUDIT.md](PATCH_READINESS_AUDIT.md) | Detailed patch applicability audit |
| [BASELINE.md](BASELINE.md) | Exact base Emacs revision and patch application |
| [BUG_COVERAGE.md](BUG_COVERAGE.md) | Verified coverage of performance pathologies |
| [DESIGN_SPACE.md](DESIGN_SPACE.md) | Design axes and variant rationale |
| [manifest.json](manifest.json) | Machine-readable index of all patches |
| [docs/coverage-claims.md](docs/coverage-claims.md) | What may and may not be claimed |
| [docs/testing-methodology.md](docs/testing-methodology.md) | How tests are run |
| [docs/benchmark-methodology.md](docs/benchmark-methodology.md) | How benchmarks are designed |
| [docs/selection-guide.md](docs/selection-guide.md) | Choosing the right variant |
| [issues/](issues/) | Documented pathologies with evidence |

---

## Optimization Families

### 1. NSColor Cache (`families/ns-color-cache/`)

Caches `NSColor` objects keyed by packed pixel value in `+colorWithUnsignedLong:`.
Eliminates per-glyph colorspace conversion on the NS/macOS drawing path.

**1 baseline + 8 variants = 9 implementations** (all macOS-only; variants are specifications)

### 2. Malloc Pressure Relief (`families/malloc-pressure-relief/`)

Calls `malloc_zone_pressure_relief(NULL, 0)` after GC to reclaim fully-free
pages on macOS (SYSTEM_MALLOC).

**1 baseline + 7 variants = 8 implementations** (macOS API; variants are specifications)

### 3. Regex ASCII Syntax Cache (`families/regex-syntax-cache/`)

Flat array caching syntax codes for ASCII characters in `re_pattern_buffer`,
replacing `syntax_property()` calls in word-boundary opcodes.

**1 baseline + 6 variants = 7 implementations** (cross-platform; variants are specifications)

### 4. Regex ASCII Translate Cache (`families/regex-translate-cache/`)

Flat array caching translation results in `re_pattern_buffer`,
replacing `char_table_translate()` calls in case-fold match paths.

**1 baseline + 6 variants = 7 implementations** (cross-platform; variants are specifications)

### 5. Regex Combined Cache (`families/regex-combined-cache/`)

Applies both regex caches (syntax + translate) in a single non-conflicting patch.

**1 baseline + 6 variants = 7 implementations** (cross-platform; variants are specifications)

---

## Verified Bug Coverage

Five performance pathologies are documented with evidence:

| ID | Issue | Level | Platform |
|---|---|---|---|
| LOCAL-PERF-001 | syntax_property hot-path overhead in word-boundary opcodes | 4 (Benchmarked) | all |
| LOCAL-PERF-002 | char_table_translate hot-path overhead in case-fold loops | 4 (Benchmarked) | all |
| LOCAL-PERF-003 | Combined regex cache composition and cumulative overhead | 4 (Benchmarked) | all |
| LOCAL-PERF-004 | NSColor per-call allocation in +colorWithUnsignedLong: | 3 (Regression-tested) | macOS |
| LOCAL-PERF-005 | RSS ratchet from malloc zone free-list after GC | 3 (Regression-tested) | macOS |

See [BUG_COVERAGE.md](BUG_COVERAGE.md) for the full report.

---

## Repository Structure

```
archive/original/           Original patches, preserved verbatim
families/
  <family>/
    baseline/
      implementation.patch  Applicable baseline patch (verified with git apply --check)
      metadata.yaml         Machine-readable metadata
      DESIGN.md             Design rationale
      STATUS.md             Apply/build/known-issues status
    variants/
      <variant>/            Same four files; all are SPECIFICATIONS (index 0000000000)
issues/
  LOCAL-PERF-NNN/           Documented performance pathology
    README.md               Description, evidence, and analysis
    metadata.yaml           Machine-readable metadata
    reproduce.sh            Deterministic reproduction script
    baseline-results/       Measured unpatched behavior
    candidate-fixes/        Patches (primary and alternatives)
    tests/                  Standalone C regression tests
    benchmarks/             Standalone C microbenchmarks + results
    build-logs/             git apply --check logs
    STATUS.md               Coverage level and status
specifications/             Design specifications moved out of candidate-fix status
combinations/               Non-conflicting multi-family patches
benchmarks/                 High-level benchmark scripts (require Emacs binary)
tests/                      Logic unit tests (no Emacs build required)
tools/                      Automation scripts
docs/                       Methodology and policy documents
.github/workflows/          CI validation
```

---

## Applying Patches

All patches apply to Emacs commit `3ca168b80ae6d7b25fe55784dde3ad24faff7be2`
(Emacs 31 development, 2026-06-23). See [BASELINE.md](BASELINE.md).

```sh
# Create a worktree at the base commit:
git -C /path/to/emacs-git worktree add /tmp/emacs-base \
  3ca168b80ae6d7b25fe55784dde3ad24faff7be2

# Apply a baseline patch:
git -C /tmp/emacs-base apply families/regex-translate-cache/baseline/implementation.patch

# Or use the automation tool:
python3 tools/apply_candidate.py \
  --patch families/regex-translate-cache/baseline/implementation.patch \
  --emacs-dir /path/to/emacs-git
```

---

## Running Tests

Standalone C tests require only a C compiler:

```sh
# Run all issue regression tests:
python3 tools/run_issue_tests.py

# Run individual tests:
cc -O2 -o /tmp/t issues/LOCAL-PERF-001/tests/test_regression.c && /tmp/t
cc -O2 -o /tmp/t issues/LOCAL-PERF-002/tests/test_regression.c && /tmp/t
cc -O2 -o /tmp/t issues/LOCAL-PERF-003/tests/test_regression.c && /tmp/t
cc -O2 -o /tmp/t issues/LOCAL-PERF-004/tests/test_cache_logic.c && /tmp/t
cc -O2 -o /tmp/t issues/LOCAL-PERF-005/tests/test_threshold_logic.c && /tmp/t

# Existing family-level tests:
cc -O2 -o /tmp/t tests/regex-syntax-cache/test_syntax_cache.c && /tmp/t
cc -O2 -o /tmp/t tests/regex-translate-cache/test_translate_cache.c && /tmp/t
cc -O2 -o /tmp/t tests/malloc-pressure-relief/test_threshold_logic.c && /tmp/t
```

## Running Benchmarks

```sh
python3 tools/run_benchmark.py --warmup 3 --runs 10
# or individually:
cc -O2 -lm -o /tmp/b issues/LOCAL-PERF-001/benchmarks/bench_syntax_overhead.c && /tmp/b
```

---

## Validation

```sh
python3 tools/validate_issue_metadata.py  # all metadata valid
python3 tools/find_base_revision.py       # confirms base commit
python3 tools/generate_bug_coverage.py    # regenerates BUG_COVERAGE.md
python3 tools/summarize_results.py        # prints benchmark summary table
```

---

## Inventory

| | Count |
|---|---|
| Baseline implementations (real patches, git apply verified) | 5 |
| Variants — design specifications (not applicable diffs) | 33 |
| Cross-family compositions | 2 |
| **Total patch artifacts** | **40** |
| Documented performance pathologies (issues/) | 5 |
| Issues at Level 3+ (regression-tested or benchmarked) | 5 |
| Issues at Level 4 (benchmarked with microbenchmarks) | 3 |

---

## Audit Summary

**Composition errors in original patches (fixed):**

1. `re_ascii_caches_0001.patch` — incorrectly contained both regex caches and
   unrelated alloc.c content. Fixed: `families/regex-syntax-cache/baseline/`
   contains only the syntax cache.

2. `ascii_caches_combined_0001.patch` — contained unrelated alloc.c and nsterm.m
   sections. Fixed: `families/regex-combined-cache/baseline/` contains only the
   regex sections. Also had formatting corruption (double bare newlines, wrong hunk
   count). Regenerated clean 2026-06-26.

See [AUDIT.md](AUDIT.md) for full analysis.
