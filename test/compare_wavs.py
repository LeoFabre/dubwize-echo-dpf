#!/usr/bin/env python3
"""Compare two 16-bit PCM WAV files sample-by-sample and report the residual.

Usage:
    compare_wavs.py <wav_a> <wav_b> [--threshold-db -80]

Exits 0  if peak residual ≤ threshold (PASS).
Exits 1  if peak residual >  threshold (FAIL).
Exits 2  on usage / file errors.

Residual is computed as (a − b) per sample across the min-common length.
Both peak and RMS are reported in dBFS (reference = full-scale 16-bit = 32768).
"""

import argparse
import math
import struct
import sys
import wave


# ── Helpers ───────────────────────────────────────────────────────────────────

def read_wav_16(path: str):
    """Return (nchannels, framerate, samples_float) for a 16-bit PCM WAV.

    samples_float is a flat list in interleaved channel order, normalised to
    [-1, 1) by dividing by 32768.
    """
    try:
        with wave.open(path, "rb") as w:
            nchan   = w.getnchannels()
            width   = w.getsampwidth()
            fs      = w.getframerate()
            nframes = w.getnframes()
            raw     = w.readframes(nframes)
    except FileNotFoundError:
        print(f"ERROR: file not found: {path}", file=sys.stderr)
        sys.exit(2)
    except wave.Error as exc:
        print(f"ERROR reading {path}: {exc}", file=sys.stderr)
        sys.exit(2)

    if width != 2:
        print(f"ERROR: {path} is {width*8}-bit PCM; expected 16-bit.", file=sys.stderr)
        sys.exit(2)

    n_samples = len(raw) // 2
    samples   = struct.unpack(f"<{n_samples}h", raw)
    return nchan, fs, [s / 32768.0 for s in samples]


def dbfs(linear: float) -> float:
    """Convert a linear amplitude (16-bit-normalised) to dBFS.

    Returns -999.0 when the input is effectively zero (guards log10(0)).
    """
    if linear < 1e-12:
        return -999.0
    return 20.0 * math.log10(linear)


# ── Main ──────────────────────────────────────────────────────────────────────

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Sample-by-sample residual comparison of two 16-bit PCM WAVs."
    )
    parser.add_argument("wav_a", help="Reference WAV (e.g. JUCE output)")
    parser.add_argument("wav_b", help="Test WAV (e.g. DPF output)")
    parser.add_argument(
        "--threshold-db",
        type=float,
        default=-80.0,
        metavar="DB",
        help="Peak residual threshold in dBFS (default: -80). "
             "Exit 1 if peak exceeds this.",
    )
    args = parser.parse_args()

    a_ch, a_fs, a_samp = read_wav_16(args.wav_a)
    b_ch, b_fs, b_samp = read_wav_16(args.wav_b)

    # ── Validate headers ──────────────────────────────────────────────────────
    if a_ch != b_ch:
        print(
            f"ERROR: channel-count mismatch — {args.wav_a}: {a_ch}ch, "
            f"{args.wav_b}: {b_ch}ch",
            file=sys.stderr,
        )
        sys.exit(2)
    if a_fs != b_fs:
        print(
            f"ERROR: sample-rate mismatch — {args.wav_a}: {a_fs} Hz, "
            f"{args.wav_b}: {b_fs} Hz",
            file=sys.stderr,
        )
        sys.exit(2)

    n_ch = a_ch
    len_a = len(a_samp)
    len_b = len(b_samp)
    n     = min(len_a, len_b)

    if len_a != len_b:
        frames_a = len_a // n_ch
        frames_b = len_b // n_ch
        shorter  = args.wav_a if len_a < len_b else args.wav_b
        print(
            f"NOTE: length mismatch ({frames_a} vs {frames_b} frames); "
            f"comparing first {n // n_ch} frames (aligned to shorter: {shorter})."
        )

    # ── Compute residual ──────────────────────────────────────────────────────
    peak_linear = 0.0
    sq_sum      = 0.0
    for i in range(n):
        diff = a_samp[i] - b_samp[i]
        mag  = abs(diff)
        if mag > peak_linear:
            peak_linear = mag
        sq_sum += diff * diff

    rms_linear = math.sqrt(sq_sum / n) if n > 0 else 0.0

    peak_db = dbfs(peak_linear)
    rms_db  = dbfs(rms_linear)

    # ── Report ────────────────────────────────────────────────────────────────
    n_frames = n // n_ch
    print(f"Files     : {args.wav_a}  vs  {args.wav_b}")
    print(f"Format    : {n_ch}ch @ {a_fs} Hz, 16-bit PCM")
    print(f"Frames    : {n_frames} compared")
    print(f"Peak residual : {peak_db:+.2f} dBFS  (linear: {peak_linear:.6e})")
    print(f"RMS  residual : {rms_db:+.2f} dBFS  (linear: {rms_linear:.6e})")
    print(f"Threshold     : {args.threshold_db:+.2f} dBFS  (peak)")

    if peak_db <= args.threshold_db:
        print("Result    : PASS")
        return 0
    else:
        print(
            f"Result    : FAIL  (peak {peak_db:+.2f} dB exceeds "
            f"threshold {args.threshold_db:+.2f} dB)"
        )
        return 1


if __name__ == "__main__":
    sys.exit(main())
