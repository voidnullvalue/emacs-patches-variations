# Status: Malloc Pressure Relief — Manual Lisp Primitive
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Apply

```sh
patch -p1 --fuzz=3 < families/malloc-pressure-relief/variants/manual-lisp-primitive/implementation.patch
```

## Build

macOS only for the non-nil return value; the defun is registered on all
platforms (returns nil on non-macOS). No extra dependencies.

## Known Issues

- The `defsubr` registration site line number is approximate; locate the
  correct `syms_of_alloc` position in the actual source.
- `make_int(result)` requires `result` to fit in a Lisp fixnum or bignum.
  `size_t released` on macOS is at most physical RAM (< 2^48 bytes), which
  fits in a Lisp bignum. The `intmax_t` clamp in the patch handles this.
- No rate-limiting: calling `(malloc-zone-pressure-relief)` in a tight loop
  walks the arena repeatedly. If called from `post-gc-hook`, the baseline's
  time-gating should be reproduced in Lisp if needed.

## Testing

- `(malloc-zone-pressure-relief)` should return a non-negative integer on macOS.
- On non-macOS builds, it should return nil without signaling an error.
- Calling it multiple times in succession should not crash or hang.
