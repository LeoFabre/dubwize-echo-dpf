#!/usr/bin/env python3
"""Generate a 10-second stereo 48 kHz 16-bit PCM WAV containing a log sweep
(20 Hz → 20 kHz) summed with pink-ish noise (single-pole IIR approximation).

Pure stdlib — no numpy/scipy required.

Output: test/fixtures/input.wav (written next to this script, so CWD-agnostic).
"""

import math
import os
import random
import struct
import wave

# ── Signal parameters ──────────────────────────────────────────────────────────
FS       = 48000
DURATION = 10.0          # seconds
N        = int(FS * DURATION)
SEED     = 42            # deterministic

# ── Pink-ish noise (1-pole IIR low-pass on white) ─────────────────────────────
# L and R use different seeds and a slight phase offset on the sweep for
# decorrelation without harming determinism.

def make_pink(rng: random.Random, n: int) -> list:
    """Single-pole IIR low-pass on white noise → pink approximation."""
    out = [0.0] * n
    b = 0.0
    for i in range(n):
        w = rng.gauss(0.0, 0.2)
        b = 0.99 * b + 0.01 * w
        out[i] = b * 10.0          # scale up so it is audible
    return out

rng_L = random.Random(SEED)
rng_R = random.Random(SEED + 1)   # different seed → decorrelated noise

pink_L = make_pink(rng_L, N)
pink_R = make_pink(rng_R, N)

# ── Log sweep 20 Hz → 20 kHz ──────────────────────────────────────────────────
FREQ_START = 20.0
FREQ_END   = 20000.0
SWEEP_AMP  = 0.25

sweep_L = [0.0] * N
sweep_R = [0.0] * N
phase_L = 0.0
phase_R = 0.0          # R gets a small initial offset for decorrelation
phase_R_offset = 0.05  # radians

for i in range(N):
    t    = i / FS
    freq = FREQ_START * (FREQ_END / FREQ_START) ** (t / DURATION)
    dph  = 2.0 * math.pi * freq / FS
    phase_L += dph
    phase_R += dph
    sweep_L[i] = SWEEP_AMP * math.sin(phase_L)
    sweep_R[i] = SWEEP_AMP * math.sin(phase_R + phase_R_offset)

# ── Mix ───────────────────────────────────────────────────────────────────────
left  = [pink_L[i] + sweep_L[i] for i in range(N)]
right = [pink_R[i] + sweep_R[i] for i in range(N)]

# ── Normalise to ~0.5 peak (safe from clipping) ───────────────────────────────
peak = max(max(abs(x) for x in left), max(abs(x) for x in right), 1e-9)
scale = 0.5 / peak

left  = [x * scale for x in left]
right = [x * scale for x in right]

# ── Write WAV ────────────────────────────────────────────────────────────────
out_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "input.wav")

frames = bytearray()
for i in range(N):
    l = max(-1.0, min(1.0, left[i]))
    r = max(-1.0, min(1.0, right[i]))
    frames += struct.pack("<hh", int(l * 32767), int(r * 32767))

with wave.open(out_path, "wb") as w:
    w.setnchannels(2)
    w.setsampwidth(2)
    w.setframerate(FS)
    w.writeframes(bytes(frames))

size_bytes = os.path.getsize(out_path)
peak_L = max(abs(x) for x in left)
peak_R = max(abs(x) for x in right)
print(f"wrote {out_path}")
print(f"  duration : {DURATION:.1f} s")
print(f"  channels : stereo")
print(f"  rate     : {FS} Hz")
print(f"  frames   : {N}")
print(f"  size     : {size_bytes:,} bytes ({size_bytes/1024/1024:.2f} MB)")
print(f"  peak L   : {peak_L:.4f} ({20*math.log10(max(peak_L,1e-12)):+.1f} dBFS)")
print(f"  peak R   : {peak_R:.4f} ({20*math.log10(max(peak_R,1e-12)):+.1f} dBFS)")

if __name__ == "__main__":
    pass  # all work is done at module level; entry-point guard for import safety
