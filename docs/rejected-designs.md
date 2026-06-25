# Rejected Designs
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

This document records design ideas that were considered and rejected because they
are cosmetically distinct from existing variants without being materially distinct,
are semantically equivalent to an existing variant, or fail to change any of the
following axes: algorithm, cache structure, cache scope, object lifetime, ownership
model, eviction policy, collision policy, invalidation strategy, trigger policy,
data representation, allocation strategy, integration point, or control-flow
specialization.

---

## Family: ns-color-cache

### Rejected: Different constant for cache size (e.g., 16384 instead of 32768)
**Reason:** Changing `NS_COLOR_CACHE_SIZE` from 32768 to any other power-of-2
does not change the algorithm, data structure, eviction policy, hash function,
or any other design axis. It is a cosmetic constant change.

### Rejected: Renaming ns_color_cache to ns_pixel_cache
**Reason:** Identifier rename with no behavioral change.

### Rejected: Inline the Fibonacci hash in `+colorWithUnsignedLong:` instead of a helper function
**Reason:** Identical algorithm and data structure; function-boundary change only.

### Rejected: Use `NSUInteger` instead of `unsigned long` for the key
**Reason:** On Apple platforms `NSUInteger` and `unsigned long` are the same type.
No behavioral change.

### Rejected: Two-level cache (global + per-frame)
**Reason:** While architecturally interesting, a two-level cache is not
materially distinct from two separate variants (per-frame + global) applied
together. The interaction is additive, not architectural. Document as a
possible composition rather than a variant.

---

## Family: malloc-pressure-relief

### Rejected: Threshold of 2 MB instead of 4 MB (static constant change)
**Reason:** Changing `MEMORY_COMPACT_FREED_THRESHOLD` from 4 MB to any other
value does not change the trigger policy algorithm, data structures, or any
other design axis. It is a constant change with no architectural significance.

### Rejected: Call `malloc_zone_compact()` instead of `malloc_zone_pressure_relief()`
**Reason:** `malloc_zone_compact()` is not part of the documented public Apple
API. Using undocumented APIs is explicitly excluded by the variant requirements.

### Rejected: Using `dispatch_after` with a fixed delay instead of idle-triggered
**Reason:** `dispatch_after` executes on a GCD queue after a wall-clock delay,
which is architecturally equivalent to the background-dispatch variant (same
threading model, same non-deterministic execution point). The delay parameter
differs but the design dimensions (async dispatch, GCD thread) are identical.

---

## Family: regex-syntax-cache

### Rejected: Using `memcpy` instead of a loop to populate the syntax cache
**Reason:** `memcpy` cannot be used here directly because `syntax_property(i, 0)`
must be called for each `i`; the syntax values cannot be copied from a flat array
without first calling the function. A loop is not semantically equivalent to
memcpy in this context; the loop-vs-memcpy distinction is not an architectural axis.

### Rejected: Cache syntax for characters 128–256 only (no ASCII)
**Reason:** The entire motivation for the cache is that ASCII is the most
common case in code buffers. A cache that covers only non-ASCII characters
inverts the optimization target and provides no benefit for the stated workload.
This is a non-meaningful variant.

### Rejected: Use `uint16_t` entries for syntax codes instead of `unsigned char`
**Reason:** `enum syntaxcode` values fit in 4 bits; `unsigned char` (8 bits)
provides adequate storage without truncation. Widening to `uint16_t` doubles the
memory without changing coverage, algorithm, validity, or invalidation strategy.
This is a type-widening change with no architectural significance. (Compare with
regex-translate-cache/identity-sentinel, where `uint16_t` enables a sentinel
value that is materially distinct in data representation.)

---

## Family: regex-translate-cache

### Rejected: Pre-compute lower-case mapping table separately from translate table
**Reason:** The translate table in Emacs already encodes case folding. A
separate lower-case mapping table would duplicate the translate cache with a
different population source but identical usage. Not materially distinct from
the baseline.

### Rejected: Use `int32_t` entries (wider than needed for byte values)
**Reason:** `int32_t` entries would store the same values as `unsigned char`
entries for the byte range 0–255. No sentinel value, no wider coverage, no
architectural difference. Pure widening without semantic change.

---

## Family: regex-combined-cache

### Rejected: Store syntax and translate in separate arrays within the combined cache (identical to baseline)
**Reason:** The baseline combined cache already uses two separate arrays. Any
variant that also uses two separate arrays (even with different naming) is not
materially distinct from the baseline unless it changes a different axis
(validity, invalidation, allocation, etc.).

### Rejected: Combined cache with a single `bool_bf valid : 1` covering both subcaches (identical to baseline)
**Reason:** The baseline already uses `bool_bf syntax_ascii_valid` and
`bool_bf trt_ascii_valid` separately. A combined single-flag variant would
reduce granularity (both invalid or both valid), which is a net-negative change
with no architectural benefit. Not materially distinct if it otherwise matches
the baseline layout.
