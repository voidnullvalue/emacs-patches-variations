# Selection Guide: Choosing a Variant
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
     Human review status: not-reviewed -->

This guide helps you choose which patch variant to use for each optimization
family, based on your priorities.

---

## Quick Decision Table

| Family | For production use | For lower memory | For correctness | For benchmarking |
|---|---|---|---|---|
| regex-syntax-cache | baseline | sidecar-allocation* | lazy-no-presexp-guard* | baseline |
| regex-translate-cache | baseline | ascii-128* | wide-entry* | every-gc-control* |
| regex-combined-cache | baseline | combined-sidecar* | independently-lazy* | baseline |
| ns-color-cache | baseline | single-entry-hot-cache* | per-display-backend* | nscache* |
| malloc-pressure-relief | baseline | effectiveness-backoff* | idle-triggered* | every-gc-control* |

`*` = design specification only (not a real applicable patch)

---

## regex-syntax-cache: Baseline

**Use when:** You want the simplest fix with no per-call overhead.

**Tradeoff:** 128 bytes added to `re_pattern_buffer` per compiled pattern.
On workloads with thousands of compiled patterns, consider `sidecar-allocation`.

**When baseline does NOT engage:**
- When `parse_sexp_lookup_properties` is non-nil (per-character syntax properties
  in use). Common with some mode-specific text property hooks.
- After pattern recompilation (cache is invalidated and rebuilt each time).

---

## regex-translate-cache: Baseline

**Use when:** You do any case-fold searching (isearch, query-replace, occur).
This is the most directly impactful patch for interactive use.

**Tradeoff:** 256 bytes per `re_pattern_buffer`. For patterns with no translate
table (nil), the initialization is skipped.

**When baseline does NOT engage:**
- When no translate table is set (non-case-fold searches).
- For characters >= 128 (multibyte; these always go through char_table_translate).

---

## regex-combined-cache: Baseline

**Use when:** You want both syntax and translate caches without applying two
conflicting patches.

**Tradeoff:** 386 bytes per pattern (256 translate + 128 syntax + 2 flags).
Uses the regenerated clean patch (2026-06-26); the original `ascii_caches_combined`
had formatting corruption.

---

## ns-color-cache: Baseline (macOS only)

**Use when:** You see high `+colorWithUnsignedLong:` overhead in Instruments
while scrolling through syntax-highlighted code on macOS.

**Avoid when:** You frequently connect and disconnect external displays. The
baseline cache never evicts, so display-specific color objects persist.
Use `per-display-backend` in that case.

---

## malloc-pressure-relief: Baseline (macOS only)

**Use when:** Your macOS Emacs session accumulates RSS over hours and you want
automatic reclamation after large GCs.

**Avoid when:** GC latency is critical (the baseline is synchronous; use
`idle-triggered` or `background-dispatch` for zero GC latency).

**Note:** Not a moving collector; cannot fix internal fragmentation.

---

## Compositions

### ns-color-cache + malloc-pressure-relief
`combinations/ns-color-cache+malloc-pressure-relief.patch`
Both baseline patches applied together. Recommended for macOS when you want
both benefits. No conflicts (different files).

### regex-combined-cache (includes both regex caches)
`families/regex-combined-cache/baseline/implementation.patch`
The only correct way to apply both regex caches. Do not apply regex-syntax-cache
and regex-translate-cache independently — they conflict at the struct level.
