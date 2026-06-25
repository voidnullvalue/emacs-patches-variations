# Status: Regex Combined Cache Baseline
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/regex-combined-cache/baseline/implementation.patch
```

## Build

Not tested. Requires Emacs source at correct base revision. If applying succeeds,
the build requires no additional flags beyond the normal Emacs configure/make.

## Known Issues

The reconstructed patch omits the erroneous `src/alloc.c` and `src/nsterm.m`
sections from the original `ascii_caches_combined_0001.patch`. If those are also
desired, apply the respective family baselines separately.

## Notes

Do not apply alongside `families/regex-syntax-cache/baseline` or
`families/regex-translate-cache/baseline` — this patch is the canonical combined
version that includes both.
