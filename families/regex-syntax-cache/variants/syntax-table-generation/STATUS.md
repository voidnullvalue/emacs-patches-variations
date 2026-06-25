# Status: Regex Syntax Cache — Syntax-Table Generation Counter
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/regex-syntax-cache/variants/syntax-table-generation/implementation.patch
```

## Build

Modifies three files: `regex-emacs.c`, `regex-emacs.h`, `syntax.c`.
The `syntax.c` hunk requires locating all mutation sites; the patch shows
representative sites only.

## Known Issues

- `uint32_t` overflow at 2^32 can cause a false cache hit (stale cache used).
  Document this in the code; in practice, 4 billion syntax mutations cannot
  occur in a normal session.
- The generation counter is process-global: a syntax change in buffer A
  invalidates all caches including those built for buffer B's syntax table.
  This is conservative but does not cause incorrect results — only extra
  cache rebuilds.
- All syntax mutation sites must be instrumented. Missing a site causes the
  cache to serve stale values. Audit `Fmodify_syntax_entry`, `Fset_syntax_table`,
  and any `Fset_char_table_range` calls that target syntax tables.

## Testing

- Compile pattern, then call `Fmodify_syntax_entry` → generation increments →
  next match should fall through to SYNTAX(c).
- After `re_compile_pattern` on the modified table → `gen_built` updated →
  fast path active again.
- Buffer-local syntax table: switch buffer → check that mismatch is detected.
