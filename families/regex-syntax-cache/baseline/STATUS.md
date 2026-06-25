# Status: Regex Syntax Cache Baseline
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/regex-syntax-cache/baseline/implementation.patch
```

The `--fuzz=3` flag is required because this is a reconstructed patch with
approximate line numbers (see AUDIT.md Note 1).

## Build

Not tested. Requires Emacs source at the correct base revision.

## Known Issues

The reconstructed patch has approximate `@@ -old,count +new,count @@` line
numbers. The content is correct but the patch may require manual adjustment if
`--fuzz=3` is insufficient. Use `patch --dry-run` first to check.

## Performance Notes

Original README reports approximately 20% improvement in font-locking scenarios.
This is a local measurement; actual improvement varies with:
- Proportion of ASCII vs non-ASCII characters in the buffer
- Whether `parse_sexp_lookup_properties` is set
- CPU architecture (cache line size, branch predictor behavior)
