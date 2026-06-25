# Design: Malloc Pressure Relief — Manual Lisp Primitive
<!-- LLM-assisted: Claude Code (claude-sonnet-4-6), 2026-06-25 -->

## Motivation

Every other variant in this family invokes `malloc_zone_pressure_relief`
automatically, with various trigger policies. This variant takes the opposite
approach: expose the API as a Lisp-callable function and let the user or Lisp
program decide when to call it.

This is the minimal integration: Emacs gains the capability without any
automatic behavior. A user can wire it to `post-gc-hook` to get baseline
behavior, or to any other hook to match their workload.

```elisp
;; Equivalent to the baseline after-GC trigger:
(add-hook 'post-gc-hook #'malloc-zone-pressure-relief)

;; Or just call it on demand:
(malloc-zone-pressure-relief)
```

## API Design

```
(malloc-zone-pressure-relief) → INTEGER | nil
```

- On macOS: calls `malloc_zone_pressure_relief(NULL, 0)` and returns the
  number of bytes reported released.
- On other platforms: returns nil (no compile-time error required in Lisp).
- Return value: non-negative integer, clamped to `intmax_t` range (the physical
  limit is total-RAM, orders of magnitude below `INTMAX_MAX`).

## Non-macOS Behavior

Both macOS and non-macOS builds register the defun. On non-macOS, the function
body returns `Qnil`. This avoids requiring Lisp callers to guard with
`(when (featurep 'ns) ...)` — the function is always defined, always safe to call.

## Docstring

The implementation includes a docstring describing:
- The underlying API used (`malloc_zone_pressure_relief (NULL, 0)`).
- The "best-effort" nature of the return value.
- The availability restriction (macOS only for a non-nil return).
- The absence of undocumented APIs.

## Comparison Table

| Property | Baseline | Manual primitive |
|---|---|---|
| Invocation | Automatic after GC | Manual (user/hook) |
| GC latency impact | Small (synchronous) | Zero (not called during GC) |
| Lisp-accessible | No | Yes |
| Returns bytes released | No (discard) | Yes |
| Requires user action | No | Yes |
