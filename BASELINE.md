# Baseline Configuration
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Machine-Readable Anchor

```yaml
emacs_revision:
  status: "unknown — cannot be established with confidence"
  evidence_class: "blob_hash"
  blob_hashes:
    src/alloc.c:       "5fc166dbc2"
    src/nsterm.m:      "a7f3fc292c"
    src/regex-emacs.c: "d8ed8a462a"
    src/regex-emacs.h: "bcfff662f7"
  contextual_bounds:
    earliest_possible: "emacs-28.1"
    latest_likely: "emacs-30.0-pre"
    best_guess: "emacs-29.x or early emacs-30 development branch"
    reasoning: >
      alloc.c contains malloc_probe, byte_ct, and total_bytes_of_live_objects
      (all post-28); alloc.c line 6012 is the GC profiling block consistent
      with the 29.x line count; regex-emacs.c uses CHAR_TO_BYTE_SAFE (29+).
  verification: >
    Run `git cat-file -e <hash>` in the Emacs source tree to verify each blob.
    The repo is at https://git.savannah.gnu.org/git/emacs.git
```

## How to Find the Exact Revision

```sh
# In a local Emacs git checkout:
git log --all --find-object=5fc166dbc2 -- src/alloc.c
git log --all --find-object=a7f3fc292c -- src/nsterm.m
git log --all --find-object=d8ed8a462a -- src/regex-emacs.c
git log --all --find-object=bcfff662f7 -- src/regex-emacs.h
```

If all four commands agree on a commit (or a narrow range of commits between
which one file changed), that is the base revision.

## Applying Patches

The baseline patches under each family's `baseline/` directory apply cleanly to
the Emacs revision identified above. Reconstructed baselines (regex-syntax-cache,
regex-combined-cache) have approximate line numbers; use `--fuzz=3`:

```sh
patch -p1 --fuzz=3 < families/regex-syntax-cache/baseline/implementation.patch
```

Variant patches are design-space explorations. They apply to the same base but
have not been tested against a live Emacs build. Treat them as implementation
specifications rather than immediately applicable patches.
