# Provenance
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Purpose

This repository is a **defensive publication and reproducible performance research
atlas** for a set of Emacs performance optimizations. Its goal is to place these
optimizations and their design-space variants clearly in the public record, with
full disclosure of provenance.

## LLM Assistance

Every implementation, variant patch, supporting script, test, benchmark, and
document in this repository (except files under `archive/original/`) was produced
with direct, substantial assistance from a large language model.

- **System:** Claude Code
- **Model:** claude-sonnet-4-6 (Anthropic)
- **Date range:** 2026-06-25 onwards
- **Role:** All patch authorship, variant design, documentation, and tooling.
  The human author provided direction, reviewed outputs, and made final decisions,
  but did not write any patch or document text independently.

## What This Repository Does NOT Do

- It does not contact Emacs maintainers or submit anything upstream.
- It does not automate upstream submissions.
- It does not obscure the LLM-assisted provenance of any file.
- It does not claim that publication here legally prevents independent
  implementations of the same ideas by others.
- It does not claim that the stated performance numbers are upstream-quality
  benchmark results.

## What This Repository Does

- It places the optimizations and their design variations in the public record
  with an explicit timestamp and authorship disclosure.
- It provides reproducible benchmarks and tests for local evaluation.
- It documents the engineering design space so that future independent
  implementors can understand the trade-offs explored.

## Original Patch Authorship

The patches in `archive/original/` were originally developed with LLM guidance
before this repository was restructured. They are preserved verbatim.

## Upstream Policy

The Emacs project does not accept LLM-assisted contributions. Nothing in this
repository is intended for or suitable for upstream submission. Reviewers and
maintainers should treat this repository as an independent research artifact only.
