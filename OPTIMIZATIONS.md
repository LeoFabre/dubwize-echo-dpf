# Optimization notes

This document describes the CPU optimization work done on the Dubwize Echo DSP for
the production setup on Bela (PocketBeagle2, Cortex-A53, aarch64). Builds use
`-O3 -march=armv8-a -mtune=cortex-a53`. All optimizations were validated with an
offline A/B regression gate (the same audio rendered through two git refs, float
dumps compared in dBFS), unit tests, and real-device benchmarks on the Cortex-A53.

**Bottom line: 2363 → 1733 ns/sample on the Cortex-A53, a 1.36× speedup**, from
Tiers 1–2 only. Both fast-math and SIMD filters were rejected — and the reason
why (the BitMod XOR stage) is the most useful lesson in this repo.

All of the work described below is merged on `main`.

## What was optimized

### Tier 1 — flush-to-zero denormals (`02e923d`)

`dsp/DenormalGuard.hpp` arms FPCR.FZ (flush-to-zero) once on the audio thread at
the top of `Plugin::run()`. aarch64-only, no-op on host builds. Protects the
delay-feedback decay tails from the Cortex-A53 denormal penalty.

### Tier 2 — numerically-equivalent per-sample wins (`02e923d`)

Safe rewrites (same math, cheaper form): `pow(x, 2)` → `x*x`,
reciprocal-multiply, hoisted loop-invariant coefficients, `__restrict` on hot
buffers. Validated bit-equivalent (or below −80 dB residual) by the offline gate.

### Tape soft-clipper coefficient caching — and the regression it caused (`9f98560`)

The tape stage's soft clipper recomputed two invariant `tanh`-derived
coefficients (`biasedZero`, `compensation`) every sample even when the tape
parameters were static. The fix is change-detection caching: recompute only when
drive/bias actually change.

The **first attempt got it wrong**, and the failure is worth telling. The cached
path refreshed `biasedZero` only *while parameters were smoothing*; at steady
state it froze at the last ramping value while the original recomputed it every
sample. That tiny constant offset, subtracted into a 55%-feedback delay loop,
**accumulated to a ~−14 dB divergence** from the original.

The per-function unit test (static parameters) passed. The end-to-end A/B
null-test caught it. The corrected version (`9f98560`) recomputes on actual
parameter change, divides by `compensation` instead of multiplying by a
precomputed reciprocal, and keeps identical clamping — the result is
**bit-identical** to the uncached clipper every sample (−999 dB on the gate)
while still skipping the two invariant `tanh` calls when parameters are static.

Lesson: in a feedback structure, unit tests on isolated functions are not enough;
an integration-level null-test against a reference render is what catches
steady-state drift.

## What was tried and rejected

### Tier 3 fast-math — rejected

`-funsafe-math-optimizations` (applied successfully to sibling plugins) is
**excluded** here: FMA contraction perturbs the float values feeding the BitMod
stage, which XORs bit patterns — so a last-bit mantissa difference becomes a
completely different output sample. The result was a **−6 dB audible** difference,
not a sub-audible residual.

### NEON SIMD state-variable filters — built, wired, reverted

- `a9e8b85` built the foundation: a 3-backend `SimdF` wrapper (NEON / SSE2 /
  scalar) and `StaticSvfStereo`, which runs both channels in 2 SIMD lanes with
  shared scalar coefficients, each lane executing the identical IEEE `+ - *` SVF
  recurrence as the per-channel `StaticSvf`. An equivalence test proved it
  bit-identical with `-ffp-contract=off`, and −125 dB with default FMA
  contraction — normally far below audibility.
- `6f132b1` wired it into the feedback-position low-pass/high-pass filters.
- `41bf6ef` **reverted the wiring**: every filter position in this plugin feeds
  the feedback loop through the BitMod XOR stage, which amplifies that −125 dB
  FMA-contraction residual to **−6 / −12 dB audible**. There is no filter
  position that is safe.

`StaticSvfStereo` remains in the tree as dead code (`dsp/SvfFilter.hpp`, with its
equivalence test in `test/test_svf_stereo.cpp`) in case the topology ever
changes. A dedicated worst-case regression preset (feedback filters + BitMod
engaged) is now part of the A/B gate, so any future attempt that reintroduces
this class of error fails the gate immediately.

The general lesson: **bit-exactness requirements are set by the most
bit-sensitive consumer in the loop.** BitMod consumes raw float bit patterns, so
any transform that changes the last mantissa bit — FMA contraction, value
reassociation — is audible here, even though it is provably inaudible everywhere
else in the chain.

## Validation

- **Offline A/B regression gate**: identical input rendered through two git refs,
  float output dumps compared in dBFS. Thresholds are calibrated: bit-identical
  reads as ≈ −999 dB, sub-audible drift ≈ −85 dB, and the tape-caching bug above
  is what a real regression looks like: ≈ −14 dB.
- **Unit tests** plus the integration null-test (which caught the −14 dB
  regression the unit tests missed).
- **Real-device benchmarks**: ns/sample measured on the Cortex-A53 itself.
