# Design Space
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

This document maps the engineering design space explored by each optimization
family and explains why each variant is materially distinct.

A variant is **not** materially distinct if its only differences are names,
formatting, comments, function ordering, or inconsequential constants.
A variant **is** materially distinct when it differs in at least one of:
algorithm, cache organization, lifetime model, invalidation strategy,
trigger policy, data representation, or integration point.

---

## Family: ns-color-cache

**Problem:** `+colorWithUnsignedLong:` is called on every glyph drawn. It
allocates an autoreleased NSColor and performs colorspace conversion on every
call, even when the same pixel value was seen nanoseconds ago.

**Baseline design axes:**
- Cache structure: direct-mapped power-of-2 array
- Key: packed 32-bit pixel (unsigned long)
- Hash: Fibonacci (multiplicative)
- Eviction: never — no-evict, process-lifetime
- Scope: global (single process-wide table)
- Thread safety: none needed (NS drawing always on main thread)

| Variant | Changed axes | Tradeoff |
|---|---|---|
| baseline | — | zero infrastructure cost; collisions silently miss |
| open-addressing | structure: open-addressing with probing; eviction: overwrite-on-probe-depth-exceeded | handles collision at cost of bounded extra memory reads |
| per-frame | scope: per-NSFrame; lifetime: frame-lifetime | lower peak memory if frames have disjoint color sets; supports frame-specific color contexts |
| counted-lru | eviction: CLOCK approximation; size: 64 | bounded memory, correct eviction order; higher constant overhead per lookup |
| single-entry-hot-cache | cache_size: 1; eviction: unconditional replacement; teardown: explicit | minimum memory; near-100% hit rate when same color repeats consecutively |
| two-way-set-associative | structure: 2-way set-associative; sets: 256; eviction: per-set CLOCK | bounded 512 entries; set-associativity reduces collision vs direct-mapped |
| nsmutable-dictionary | structure: NSMutableDictionary; eviction: none; ownership: NSDictionary semantics | no hand-written hash; unbounded growth; correct retain/release via Foundation |
| nscache | structure: NSCache; eviction: automatic under memory pressure | system-managed eviction; advisory countLimit; thread-safe by contract |
| per-display-backend | scope: per-NsDisplayInfo; lifetime: backend creation/teardown | correct on multi-display; no cache leaks across display reconnects |

---

## Family: malloc-pressure-relief

**Problem:** On macOS with SYSTEM_MALLOC, freed Lisp objects stay on allocator
free lists and keep pages resident. RSS ratchets upward over long sessions.
`malloc_zone_pressure_relief(NULL, 0)` asks the OS to reclaim fully-free pages.

**Baseline design axes:**
- Trigger: threshold on freed bytes OR every-N-GCs fallback
- Timing: synchronous, called at end of `garbage_collect()`
- Rate limit: minimum wall-clock interval between compactions
- Zone scope: NULL (all zones)
- Thresholds: static constants

| Variant | Changed axes | Tradeoff |
|---|---|---|
| baseline | — | simple, predictable; adds small synchronous pause to GC |
| background-dispatch | timing: async via libdispatch; thread safety: atomic dispatch flag | zero GC-time latency; less predictable timing; needs OS thread |
| adaptive-threshold | thresholds: learned from recent release history | self-tunes to workload; avoids thrashing on low-yield builds; adds per-GC bookkeeping |
| manual-lisp-primitive | trigger: user-callable defun; automatic_invocation: false | zero GC overhead; Lisp-accessible and scriptable; requires user/hook to invoke |
| idle-triggered | timing: deferred to idle loop; pending_state: bool flag | zero GC latency; no threading; requires keyboard.c integration |
| every-gc-control | trigger: unconditional; purpose: benchmark reference | maximum possible benefit; maximum latency overhead; not for production |
| freed-byte-goal | goal_argument: freed_bytes (not 0); integer_conversion: safe | hint for early arena-walk termination; improvement depends on allocator |
| effectiveness-backoff | trigger: adaptive periodic backoff; algorithm: double interval after 3 low-yield | reduces frequency on low-yield workloads; resets after meaningful release |

---

## Family: regex-syntax-cache

**Problem:** Inside `re_match_2_internal`, the `SYNTAX(c)` macro calls
`syntax_property(c, 0)` which walks the buffer-local char-table. For ASCII
characters (the overwhelmingly common case in code buffers), the syntax code
is the same every time and could be read from a 128-byte flat array.

**Baseline design axes:**
- Cache location: `re_pattern_buffer` (per compiled pattern)
- Population: eager at `re_compile_pattern` time
- Validity guard: `!parse_sexp_lookup_properties`
- Invalidation: recompile pattern when syntax table changes
- Size: 128 bytes (ASCII only)

| Variant | Changed axes | Tradeoff |
|---|---|---|
| baseline | — | simplest; works with Emacs's existing compile-pattern invalidation |
| global-epoch | location: global table; invalidation: global epoch counter | O(1) memory regardless of pattern count; requires epoch tracking in syntax table mutation paths |
| lazy-no-presexp-guard | population: defer guard to runtime; guard: runtime position check | allows use even when parse_sexp_lookup_properties is set, with per-call position fallback |
| single-byte-256 | cache_size: 256; coverage: 0-255 | covers full byte range; 128 extra bytes per pattern; useful for unibyte buffers |
| sidecar-allocation | storage: xmalloc sidecar; struct_overhead: pointer only | reduces struct size; adds allocation cost per pattern |
| syntax-table-generation | invalidation: generation counter; mutation_sites: syntax.c | explicit invalidation; requires auditing all syntax mutation paths |
| specialized-ascii-matcher | integration_point: pre-match dispatch; specialization: unibyte word-boundary | general matcher unchanged; differential testing natural; one extra function call |

---

## Family: regex-translate-cache

**Problem:** `TRANSLATE(d)` calls `char_table_translate(translate, d)` on every
character matched during case-folding searches. For ASCII characters, the
translation result is fixed for the lifetime of the pattern.

**Baseline design axes:**
- Cache location: `re_pattern_buffer` (per compiled pattern)
- Population: eager at `re_compile_pattern` time
- Entry type: `unsigned char` (1 byte per entry)
- Coverage: 0–255 (full extended ASCII range)
- Validity: single `bool_bf` flag

| Variant | Changed axes | Tradeoff |
|---|---|---|
| baseline | — | eager fill, simple; char truncation for codes 128-255 handled by CHAR_TO_BYTE_SAFE |
| lazy-bitmap | population: lazy (on first access); validity: per-entry 256-bit bitmap | amortizes fill cost for patterns that only translate a few characters |
| wide-entry | entry type: `uint16_t`; coverage: 0–255 with full Unicode translation values | avoids CHAR_TO_BYTE_SAFE truncation; 512 bytes vs 256 bytes; allows detecting non-ASCII translation results |
| ascii-128 | coverage: 0-127; cache_size_bytes: 128 | saves 128 bytes; covers pure-ASCII case folding; Latin-1 always slow path |
| sidecar-allocation | storage: xmalloc sidecar; struct_overhead: pointer only | reduces struct size; adds allocation per pattern; table_ref for stale detection |
| identity-sentinel | entry_type: uint16_t; sentinel: 0xFFFF for identity | explicit identity encoding; no NUL ambiguity; doubles entry width for 128-entry coverage |
| table-generation | validation: EQ translate-table identity | detects table replacement; does not detect in-place mutation |

---

## Family: regex-combined-cache

**Problem:** When both syntax and translate caches are desired, applying them as
separate patches causes conflicts because both modify the same struct and the
same end-of-compile_pattern block.

**Baseline design axes:**
- Composition: independent syntax + translate caches in a single patch
- Validity: two separate `bool_bf` flags
- Initialization: two separate if-blocks in `re_compile_pattern`

| Variant | Changed axes | Tradeoff |
|---|---|---|
| baseline | — | minimal composition of the two independent caches |
| unified-generation | invalidation: single generation counter; struct: unified `cache_generation` field | single invalidation path; generation mismatch triggers full rebuild of both caches at once |
| search-path-hook | integration point: `search.c`:`compile_pattern` wrapper; cache reuse: skip recompile if pattern text and tables unchanged | amortizes compile cost across successive searches; requires search.c modification |
| per-character-struct | structure: AoS (one struct per char); validity: per-character per-subcache | fine-grained independent validity; 512 bytes; AoS vs SoA layout change |
| packed-entry | data_representation: packed uint16_t; fields: 5b syntax + 1b valid + 8b translate + 1b valid | minimum size (256 bytes); masks/shifts required; no silent truncation |
| independently-lazy | population: lazy on first use per subcache; validity: built + valid flags | skips syntax population for non-word-boundary patterns; amortizes cost |
| combined-sidecar | storage: one xmalloc for both subcaches; struct_overhead: pointer only | minimal struct size; single allocation/deallocation covers both caches |

---

## Cross-family Combinations

See `combinations/` for patches that apply multiple families together.

| Combination | Families | Design theme |
|---|---|---|
| `ns-color-cache+malloc-pressure-relief` | ns-color-cache (baseline) + malloc-pressure-relief (baseline) | non-conflicting baseline composition |
| `per-display-backend+idle-triggered` | ns-color-cache/per-display-backend + malloc-pressure-relief/idle-triggered | scoped-lifetime + deferred-work philosophy |

The regex-syntax-cache and regex-translate-cache families conflict at the struct
level (both add fields to `re_pattern_buffer`); they must be composed via the
regex-combined-cache family rather than applied independently.
