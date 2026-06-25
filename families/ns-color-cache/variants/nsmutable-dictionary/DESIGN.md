# Design: NSColor Cache — NSMutableDictionary
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

All other variants in this family implement caches using hand-written C data
structures. This variant delegates all storage, hashing, collision resolution,
and retain/release management to Foundation's `NSMutableDictionary`. The goal is
to minimize the amount of custom code while documenting the ownership semantics
and the unbounded-growth implication clearly.

This design is appropriate for:
- Research and prototyping where simplicity is valued over tuning.
- Workloads where the number of distinct colors is bounded by the theme definition
  (typically 10–60 entries) and memory growth is not a concern.

It is not appropriate for production builds on memory-constrained hardware without
adding an eviction strategy (or replacing with `NSCache`).

## Algorithm

**Initialization:** `[[NSMutableDictionary alloc] init]`. Called once from
`ns_initialize_display_info`.

**Lookup:** `ns_color_dict[@((unsigned long)c)]` — subscript via boxed NSNumber key.

**Store:** `ns_color_dict[@((unsigned long)c)] = col` — dictionary retains the value.

**Teardown:** `[ns_color_dict release]; ns_color_dict = nil`. Called from
`ns_term_shutdown`.

## Ownership Semantics

- **Keys:** Boxed NSNumber literals. NSDictionary copies keys; since NSNumber is
  immutable, copying is equivalent to retaining. The literal may be pooled (for
  small integers) or autoreleased; either way the dictionary holds a copy.
- **Values:** Retained by the dictionary. The value returned by `-objectForKey:`
  is owned by the dictionary; callers must not release it without retaining first.
- **Autorelease pools:** The `@(c)` literal for large unsigned long values may be
  autoreleased. If this function is called outside a runloop iteration with an
  active autorelease pool, wrap the store in an explicit `NSAutoreleasePool`.

## Unbounded Growth

The dictionary has no eviction policy. In a session with a fixed theme, at most
50–100 entries are ever created. The growth is bounded by the number of distinct
packed pixel values ever requested. If Emacs is used with dynamically generated
colors (image rendering, color gradients), the dictionary will grow without bound.

## Comparison Table

| Property | Baseline (direct-mapped) | NSMutableDictionary | NSCache |
|---|---|---|---|
| Eviction | None (never) | None (unbounded) | Automatic (mem pressure) |
| Memory bound | ~786 KB static | Unbounded dynamic | Soft (countLimit) |
| Key type | unsigned long (C scalar) | NSNumber object | NSNumber object |
| Code complexity | Medium (hash fn, probing) | Low | Low |
| Suitable for production | Yes (with caveats) | Only for bounded color sets | With benchmarks |
