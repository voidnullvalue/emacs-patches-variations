# Status: Regex Translate Cache Baseline
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 < families/regex-translate-cache/baseline/implementation.patch
```

## Build

Not tested. Requires Emacs source at the correct base revision.

## Known Issues

None identified in review. The patch is clean and self-contained.

## Performance Notes

Original README reports up to 50% improvement in case-folding regex scenarios.
This is a local, scenario-specific measurement. Improvement requires:
- `case-fold-search` to be non-nil (translate table present)
- High proportion of ASCII characters in the searched text
- Sufficient pattern matches to amortize the compile-time fill cost
