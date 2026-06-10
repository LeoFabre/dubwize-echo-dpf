# dubwize-echo-dpf

A headless [DPF](https://github.com/DISTRHO/DPF) port of **Dubplex Dubwize Echo** (originally a JUCE/WebView2
plugin), targeting Bela (PocketBeagle2) inside Sushi. Builds VST3 + LV2, with an optional NanoVG desktop debug UI.

The original is itself derived from *StrangeReturns* by JackWithOneEye (Apache-2.0). This port is Apache-2.0.

## Build (host)

```bash
git submodule update --init --recursive        # pulls DPF + pugl
cmake -B build-host -DDUBWIZE_BUILD_UI=OFF
cmake --build build-host
```

Headless VST3/LV2 land in `build-host/bin/`.

## Test

```bash
ctest --test-dir build-host --output-on-failure
```

## UI (desktop debug only)

```bash
cmake -B build-ui -DDUBWIZE_BUILD_UI=ON && cmake --build build-ui
```

## DSP

Header-only, allocation-free modules under `dsp/`, orchestrated by `DubwizeEngine`. JUCE math
(`FastMathApproximations`, `SmoothedValue`, `Decibels`, `Random`) is reimplemented in-tree; fast tan/sin are
replicated bit-for-bit so a noise-off null-test against the JUCE original reaches ≤ −80 dB.

## Performance

See [OPTIMIZATIONS.md](OPTIMIZATIONS.md) for the Cortex-A53 optimization work, measured gains, and the rejected approaches (fast-math, SIMD filters) and why.

## Footprint

Headless Release build (`DUBWIZE_BUILD_UI=OFF`): the VST3 binary is ~180 KB (bundle ~192 KB), well under the 1 MB
target — vs the JUCE/WebView2 original which embeds a full Chromium-backed UI and bundled React assets.
