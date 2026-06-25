# Design: Regex Syntax Cache — Global Epoch Variant
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

The baseline adds 128 bytes to every `re_pattern_buffer`. For programs that
compile many patterns (e.g., `font-lock` compiling one pattern per keyword
group), this accumulates. The global epoch variant stores a single 128-byte
array for the entire process.

## Epoch Counter Mechanism

A file-static `uintmax_t emacs_syntax_table_epoch` starts at 1 and is
incremented whenever the current buffer's syntax table changes. Sites that
must bump the epoch include:
- `set-syntax-table` (Lisp function in syntax.c)
- `modify-syntax-entry` (per-character syntax change)
- `use-local-map` (if local keymaps include syntax modifications)
- Buffer creation with a non-standard syntax table

The global cache stores `global_syntax_epoch_built`, the epoch value when the
cache was last filled. The `SYNTAX_TRU` macro checks if the two values match;
if not, it falls back to `SYNTAX(c)`.

At `re_compile_pattern` time, if the epoch has advanced since the last fill,
the cache is rebuilt by calling `syntax_property(i, 0)` for i = 0..127.

## Trade-off: One Cache vs Many

The global cache can only serve one syntax table at a time. When the user switches
to a buffer with a different syntax table, the epoch bumps and the cache is
rebuilt at the next `re_compile_pattern` call. In a typical session involving
one major mode (e.g., python-mode), the syntax table rarely changes, so the
rebuild is infrequent.

In contrast, the baseline's per-pattern cache survives buffer switches: each
pattern remembers its own syntax table snapshot. The global cache cannot
provide this — it always reflects the current buffer's syntax table.

## Integration Points

`emacs_syntax_table_epoch` must be declared in a header (e.g., `syntax.h`) and
defined in `syntax.c`. Every function that modifies the current buffer's syntax
table must call `emacs_syntax_table_epoch++` after the modification.

Failing to bump the epoch at a mutation site is a correctness bug (the cache
returns stale syntax codes). The epoch mechanism is therefore more fragile than
the baseline's recompile-based invalidation, which is automatic.
