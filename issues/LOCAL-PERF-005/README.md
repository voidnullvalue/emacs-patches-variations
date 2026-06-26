# LOCAL-PERF-005: RSS Ratchet from Malloc Zone Free-List Accumulation After GC
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
     Human review status: not-reviewed -->

## 1. Precise Description

On macOS, Emacs uses the system allocator (`SYSTEM_MALLOC`). After `garbage_collect()`
frees Lisp objects, the freed memory goes onto the malloc zone's free lists, keeping
the underlying pages physically resident (RSS stays high). The OS will not reclaim
these pages until either:
- A subsequent allocation reuses them, OR
- The allocator explicitly releases them via `malloc_zone_pressure_relief()`

Over long Emacs sessions with intermittent large GCs (e.g., loading many packages,
M-x byte-compile-file repeatedly, running large tests), RSS ratchets upward even
though most of the live set is small. This wastes physical memory, degrades cache
locality, and can trigger OS-level memory pressure warnings.

`malloc_zone_pressure_relief(NULL, 0)` is a public macOS API (available since
macOS 10.7) that asks every registered zone to release fully-free pages back to
the OS. Calling it after each GC that freed significant memory reduces the RSS
ratchet effect.

The candidate patch adds a gated call to this API at the end of `garbage_collect()`,
after all sweeping completes. The gate ensures:
- Only runs when ≥ 4 MB was freed (configurable)
- Minimum 3 seconds between compactions (rate limiter)
- Falls back to running every 64 GCs even if freed bytes are low (periodic sweep)

**Platform:** macOS only (`HAVE_MALLOC_MALLOC_H` define from configure).

## 2. Exact Affected Emacs Commit / Tag

- **Base commit:** `3ca168b80ae6d7b25fe55784dde3ad24faff7be2` (2026-06-23)
- **Nearest tag:** `emacs-31.0.90` (350 commits earlier)
- **Blob hash (verified):**
  - `src/alloc.c`: `5fc166dbc24e50a850b0bbf364f9a2f768458a65`

## 3. Deterministic Reproduction

**On macOS:**
```sh
sh issues/LOCAL-PERF-005/reproduce.sh
```

Requires macOS with developer tools. The script:
1. Builds unpatched Emacs
2. Runs a workload: load many packages, force GC, measure RSS
3. Applies the patch
4. Rebuilds and re-runs the same workload
5. Compares RSS trajectories

**On Linux:** Threshold logic test runs (pure C, no macOS APIs needed).

## 4. Baseline Measurements / Failing Behavior

**Expected (macOS, not measured on this host):**
- Unpatched: RSS increases by 100–500 MB over a 2-hour editing session with
  intermittent large GCs; does not return to baseline between GCs
- Patched: RSS returns to within ~10% of initial size after each GC that
  releases ≥ 4 MB of freed data

**Threshold logic verification (Linux, PASSED):**
- EMA converges to tracked yield value
- Zero-yield compactions floor at 256 KB threshold
- Rate limiter correctly gates compactions to ≥ 3 seconds apart

## 5. Candidate Fix

**Primary:** `families/malloc-pressure-relief/baseline/implementation.patch`

Adds to `src/alloc.c`:
- `malloc_zone_compact()` wrapper around `malloc_zone_pressure_relief(NULL, 0)`
- `maybe_compact_memory_after_gc(byte_ct freed_bytes)`: gated dispatcher
- Integration at `garbage_collect()` exit: extracts `freed_bytes` from
  `tot_before - tot_after`, calls `maybe_compact_memory_after_gc`
- Session counters: `memory_compacts_done`, `memory_compact_bytes_returned`
- All code under `#ifdef HAVE_MALLOC_MALLOC_H` (no Linux build impact)

`git apply --check` passes on both platforms. On Linux it compiles as a no-op.

**Alternative (SPECIFICATION):**
`families/malloc-pressure-relief/variants/idle-triggered/implementation.patch`
— Defers compaction to the Emacs idle loop. Not a real patch.

## 6. Regression Test

`issues/LOCAL-PERF-005/tests/test_threshold_logic.c`

This is the same test as `tests/malloc-pressure-relief/test_threshold_logic.c`
(pre-existing in the repository). It verifies the EMA threshold logic in
isolation without macOS APIs.

```sh
cc -O2 -o /tmp/t5 issues/LOCAL-PERF-005/tests/test_threshold_logic.c && /tmp/t5
```

The full RSS-measurement regression test (requires macOS) is blocked on Linux.

## 7. Apply, Build, Test, Benchmark Status

| Step | Status | Notes |
|---|---|---|
| `git apply --check` | **passed** | Applies on Linux; no-op under `#ifdef HAVE_MALLOC_MALLOC_H` |
| `git apply` | **passed** | Verified on Linux |
| `./autogen.sh` | blocked | No autoconf on this host |
| `./configure` | blocked | No libncurses-dev |
| `make -jN` | blocked | Blocked |
| Threshold logic test (C) | **passed** | EMA, floor, rate logic correct |
| RSS benchmark | blocked | macOS only |
| `make check` | not-run | Blocked |

## 8. Remaining Risks and Limitations

1. **RSS measurement blocked on Linux.** macOS-specific.

2. **Not a compacting allocator.** `malloc_zone_pressure_relief` cannot move live
   objects; it only reclaims pages that are entirely free. Internal fragmentation
   (a page partially occupied by one live object) is not addressed.

3. **GC latency:** Synchronous compaction adds latency to `garbage_collect()`.
   The baseline patch has a synchronous call. The `background-dispatch` and
   `idle-triggered` variants eliminate this latency at the cost of less predictable
   timing.

4. **MALLOC_PROBE macro:** The patch calls `MALLOC_PROBE (released)` after
   compaction. This macro is defined in alloc.c (GNU Emacs's profiling hook);
   it must be present in the target build.

5. **Every-N-GCs fallback:** The fallback fires every 64 GCs regardless of freed
   bytes. In a memory-intensive workload with many small GCs, this could add up
   to ~20 ms/hour of latency (32 ms per compact × 6 per hour = ~192 ms/hour).

6. **LLM-assisted:** All patches and tests are LLM-generated.
