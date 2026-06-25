# Design: NSColor Cache — NSCache
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

`NSMutableDictionary` provides no eviction. The `NSCache` variant adds automatic
eviction under system memory pressure at the cost of non-deterministic cache
contents. `NSCache` is appropriate when:
- Memory pressure is a concern but the exact eviction policy does not matter.
- Thread safety is required (NSCache is documented as thread-safe).
- Simplicity is valued over deterministic performance.

The tradeoff vs deterministic caches (CLOCK-LRU, set-associative) is that
`NSCache` eviction timing and choice of victim are system-controlled; the cache
may evict recently-used entries if the system decides to reclaim memory,
which can increase miss rates unexpectedly.

## Algorithm

**Initialization:** `[[NSCache alloc] init]` with `countLimit = 512`.

**Lookup:** `[ns_color_nscache objectForKey: @((unsigned long)c)]`. Returns the
cached object if present and not yet evicted; nil otherwise.

**Store:** `[ns_color_nscache setObject: col forKey: @((unsigned long)c)]`.
NSCache retains the value.

**Teardown:** `[ns_color_nscache release]; ns_color_nscache = nil`.

**Return value discipline:** The returned `col` is held by the caller's local
variable for the duration of the drawing call. Even if NSCache evicts the entry
between the store and the next lookup, the caller's reference stays valid.

## countLimit Semantics

`NSCache.countLimit` is advisory. The cache may hold more entries than the limit
if the OS has not requested memory reclamation. The limit is a hint that helps
the cache manager decide what to evict, not a strict cap. For strict caps, use a
CLOCK or set-associative C cache.

## Differences from Deterministic Caches

| Property | CLOCK-LRU (64 slots) | NSCache (512 limit) |
|---|---|---|
| Eviction trigger | Every insertion when full | System memory pressure |
| Eviction choice | Least-recently-used (approx.) | System-managed |
| Determinism | Yes | No |
| Memory bound | Strict (1.5 KB) | Soft (advisory limit) |
| Thread safety | No (manual) | Yes (by contract) |

## Thread Safety

`NSCache` is documented as thread-safe. NS drawing currently occurs on the main
thread so this property is not exercised. It provides forward-safety if the
drawing path is ever parallelized.

## Comparison Table

| Property | Baseline | NSMutableDictionary | NSCache |
|---|---|---|---|
| Eviction | Never | Never | Automatic |
| Memory | ~786 KB static | Unbounded | Soft capped |
| Determinism | Yes | Yes | No |
| Thread-safe | Yes (single-thread) | No | Yes |
