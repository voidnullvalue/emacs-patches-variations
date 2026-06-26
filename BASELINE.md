# Baseline Configuration
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-26
     Updated: 2026-06-26 — exact base commit identified -->

## Exact Base Revision

All five baseline patches were verified against **one common base commit**:

```
commit  3ca168b80ae6d7b25fe55784dde3ad24faff7be2
author  Emacs developers
date    2026-06-23
msg     Merge from origin/emacs-31
branch  master (Emacs 31 development)
tag     emacs-31.0.90 + 350 commits (post-31.0.90 pre-release)
```

This commit was identified by searching the Emacs git history for the intersection
where all four known blob hashes are simultaneously present (see below).

**Note on original estimate:** Earlier AUDIT.md and BASELINE.md stated the base
was "Emacs 29.x or early Emacs 30". This was incorrect. The actual base is
Emacs 31 development code from June 2026, well past the emacs-29.x and emacs-30.x
releases.

---

## Machine-Readable Anchor

```yaml
emacs_revision:
  status: "confirmed — exact commit identified 2026-06-26"
  commit: "3ca168b80ae6d7b25fe55784dde3ad24faff7be2"
  date: "2026-06-23"
  message: "Merge from origin/emacs-31"
  nearest_tag: "emacs-31.0.90"
  commits_after_tag: 350
  branch: "master (Emacs 31 development)"
  verification_command: >
    git -C /tmp/emacs-src ls-tree 3ca168b80ae6d7b25fe55784dde3ad24faff7be2 \
      src/alloc.c src/nsterm.m src/regex-emacs.c src/regex-emacs.h
  blob_hashes:
    src/alloc.c:
      short: "5fc166dbc2"
      full:  "5fc166dbc24e50a850b0bbf364f9a2f768458a65"
    src/nsterm.m:
      short: "a7f3fc292c"
      full:  "a7f3fc292c7fd87a067344008b7afad5ddcaa1ad"
    src/regex-emacs.c:
      short: "d8ed8a462a"
      full:  "d8ed8a462a1a2a9cf5a02f169d1127fa2aa5753b"
    src/regex-emacs.h:
      short: "bcfff662f7"
      full:  "bcfff662f7b65868ab6098338e8105ae31b5d021"
```

---

## How the Revision Was Found

Using a blobless Emacs git clone:

```sh
git clone --filter=blob:none --no-checkout \
  https://github.com/emacs-mirror/emacs.git /tmp/emacs-src

# Search commits touching each file for the matching blob hash
git -C /tmp/emacs-src log --all --format="%H" -- src/alloc.c | while read c; do
  blob=$(git -C /tmp/emacs-src ls-tree "$c" src/alloc.c | awk '{print $3}')
  [[ "$blob" == 5fc166dbc2* ]] && echo "MATCH alloc.c: $c $blob" && break
done
# Result: commit 225876e97999664a075eb6f1489b4b4c8e515ded (2026-05-26, ARRAYELTS → countof)

# Similarly for other files:
# regex-emacs.c: 5d8bb14d3b90513ed1a849ebbafb82a7734d9c8c (2026-05-26, Omit useless casts)
# nsterm.m:      2ba2024558106cbecccdb2575e6717bb97c66ea1 (2026-06-20, Merge from origin/emacs-31)
# regex-emacs.h: c31f6adc31d48076c63ad82b83b2970e1b0d7b9b (2026-01-01, copyright years)

# Then found the commit where ALL four are simultaneously present:
# 3ca168b80ae6d7b25fe55784dde3ad24faff7be2 (2026-06-23, Merge from origin/emacs-31)
```

See `tools/find_base_revision.py` for the automated version.

---

## Verification

```sh
# Verify all four blobs at the exact base commit:
commit=3ca168b80ae6d7b25fe55784dde3ad24faff7be2
git -C /tmp/emacs-src ls-tree "$commit" src/alloc.c src/nsterm.m src/regex-emacs.c src/regex-emacs.h
# Expected output:
# 100644 blob 5fc166dbc24e50a850b0bbf364f9a2f768458a65  src/alloc.c
# 100644 blob a7f3fc292c7fd87a067344008b7afad5ddcaa1ad  src/nsterm.m
# 100644 blob d8ed8a462a1a2a9cf5a02f169d1127fa2aa5753b  src/regex-emacs.c
# 100644 blob bcfff662f7b65868ab6098338e8105ae31b5d021  src/regex-emacs.h
```

```sh
# Verify all baseline patches apply to this commit:
git -C /tmp/emacs-src worktree add /tmp/emacs-base \
  3ca168b80ae6d7b25fe55784dde3ad24faff7be2

for family in regex-syntax-cache regex-translate-cache regex-combined-cache \
              malloc-pressure-relief ns-color-cache; do
  git -C /tmp/emacs-base apply --check \
    families/$family/baseline/implementation.patch && echo "OK: $family"
done
# All five exit 0 (verified 2026-06-26)
```

---

## Applying Patches

All five baseline patches apply with `git apply` (no `--fuzz` needed):

```sh
# Navigate to a checkout of the base commit, then:
git apply families/ns-color-cache/baseline/implementation.patch
git apply families/malloc-pressure-relief/baseline/implementation.patch
git apply families/regex-translate-cache/baseline/implementation.patch
git apply families/regex-syntax-cache/baseline/implementation.patch
# Note: regex-combined-cache replaces separate syntax + translate patches
git apply families/regex-combined-cache/baseline/implementation.patch
```

**Note on regex-combined-cache:** The original `ascii_caches_combined_0001.patch`
in `archive/original/` had formatting corruption (double bare blank lines, wrong
hunk line count). The baseline in `families/regex-combined-cache/` was regenerated
on 2026-06-26 and applies cleanly.

---

## Per-Family Base Revision Summary

| Family | Files modified | Base commit | Apply status |
|---|---|---|---|
| ns-color-cache | src/nsterm.m | `3ca168b80a` | passed |
| malloc-pressure-relief | src/alloc.c | `3ca168b80a` | passed |
| regex-syntax-cache | src/regex-emacs.c, src/regex-emacs.h | `3ca168b80a` | passed |
| regex-translate-cache | src/regex-emacs.c, src/regex-emacs.h | `3ca168b80a` | passed |
| regex-combined-cache | src/regex-emacs.c, src/regex-emacs.h | `3ca168b80a` | passed |

All families share the same base commit. The patches are not rebased onto
current master.
