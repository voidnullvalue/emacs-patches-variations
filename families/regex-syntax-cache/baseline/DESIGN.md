# Design: Regex Syntax Cache Baseline
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Problem

Inside `re_match_2_internal`, word-boundary and syntax-class opcodes
(`wordbeg`, `wordend`, `wordbound`, `wordbeg`, `syntaxspec`, etc.) call the
`SYNTAX(c)` macro, which expands to `syntax_property(c, 0)`. This function
performs a char-table walk to find the syntax code of character `c` in the
current buffer's syntax table.

For ASCII characters (codes 0–127), the result is identical on every call —
the buffer's syntax table is fixed for the duration of a `re_match_2_internal`
call. In font-locking scenarios, the same ASCII characters are checked thousands
of times per buffer.

## Approach

Add `syntax_ascii[128]` (an array of `unsigned char`) and `syntax_ascii_valid`
(a `bool_bf` flag) to `re_pattern_buffer`. At `re_compile_pattern` time, if
`parse_sexp_lookup_properties` is false (meaning syntax codes are uniform across
the buffer), fill `syntax_ascii` by calling `syntax_property(i, 0)` for
i = 0..127.

Replace every `SYNTAX(c)` call inside the match engine with `SYNTAX_TRU(c)`,
defined as:

```c
#define SYNTAX_TRU(c)                                          \
  ((bufp->syntax_ascii_valid && (unsigned)(c) < 128)          \
   ? (enum syntaxcode) bufp->syntax_ascii[(unsigned char)(c)] \
   : SYNTAX(c))
```

On a hot iteration where `c` is an ASCII character and the cache is valid, the
macro reduces to a single bounds check + array load, returning the result without
any function call.

## Guard: `parse_sexp_lookup_properties`

When `parse_sexp_lookup_properties` is non-nil, the syntax code of a character
can vary by buffer position (via the `syntax-table` text property). In that case
the compile-time cache is incorrect, and `syntax_ascii_valid` is set to false so
every call falls back to the full `syntax_property` path.

## Invalidation

The cache is in `re_pattern_buffer`, which Emacs recompiles whenever the syntax
table changes (search.c's `compile_pattern` wrapper tracks this). No additional
invalidation mechanism is needed.

## Limitations

- Does not help when `parse_sexp_lookup_properties` is set.
- Only covers ASCII (0–127). Non-ASCII characters always call `syntax_property`.
- 128 bytes added to every `re_pattern_buffer`. In practice this is negligible
  because the buffer already contains compiled bytecode.
