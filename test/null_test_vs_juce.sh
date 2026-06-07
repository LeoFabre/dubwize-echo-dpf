#!/usr/bin/env bash
# =============================================================================
# null_test_vs_juce.sh — Dubwize-Echo DPF null-test harness vs JUCE original
# =============================================================================
#
# Renders the same input WAV through the JUCE original VST3 and the DPF VST3
# (noise OFF for both) using an offline host, then asserts peak residual
# ≤ -80 dB.
#
# Acceptance criterion (spec §11):
#   Peak residual ≤ −80 dB for every preset.
#   A preset may be relaxed to −60 dB ONLY if a numerical justification is
#   written in a comment alongside the preset's threshold override:
#     e.g., "tape saturation adds <0.5 LSB rounding, theoretical ceiling −62 dB"
#
# Requirements (the script will fail with a clear message if any are missing):
#   1. DPF plugin built headless:
#        cd plugins/dubwize-echo-dpf && cmake -B build-host -DBUILD_UI=OFF && cmake --build build-host
#      Expected: build-host/bin/Dubwize.vst3
#   2. JUCE original built (natively, same OS/arch):
#        cd DubwizeEcho && cmake -B build && cmake --build build --config Release
#      Expected: pointed to by $DUBWIZE_JUCE_VST3
#   3. carla-single (Carla offline host) on $PATH.
#      On Debian/Ubuntu: apt install carla
#      On macOS (Homebrew): brew install carla
#      Alternative: set $OFFLINE_HOST to a wrapper script that accepts the same
#      interface: <host> vst3 <plugin.vst3> <in.wav> <out.wav> [param=value ...]
#
# Usage:
#   DUBWIZE_JUCE_VST3=/path/to/DubwizeEcho.vst3 bash test/null_test_vs_juce.sh
#
# Optional env-var overrides:
#   THRESHOLD_DB   — global peak threshold (default: -80)
#   OFFLINE_HOST   — override the rendering host binary (default: carla-single)
# =============================================================================

set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${HERE}/.." && pwd)"

# ── Paths ─────────────────────────────────────────────────────────────────────
FIXTURE="${HERE}/fixtures/input.wav"
DPF_VST3="${REPO_ROOT}/build-host/bin/Dubwize.vst3"
JUCE_VST3="${DUBWIZE_JUCE_VST3:?ERROR: Set DUBWIZE_JUCE_VST3 to the DubwizeEcho JUCE VST3 bundle path}"
THRESHOLD_DB="${THRESHOLD_DB:--80}"
OFFLINE_HOST="${OFFLINE_HOST:-carla-single}"
COMPARE_PY="${HERE}/compare_wavs.py"
OUTDIR="${HERE}/fixtures"

# ── Dependency checks ─────────────────────────────────────────────────────────
if [[ ! -e "${DPF_VST3}" ]]; then
    echo "ERROR: DPF VST3 not found at ${DPF_VST3}" >&2
    echo "       Build it with:" >&2
    echo "         cd $(dirname "${REPO_ROOT}") && " \
         "cmake -B plugins/dubwize-echo-dpf/build-host -S plugins/dubwize-echo-dpf -DBUILD_UI=OFF && " \
         "cmake --build plugins/dubwize-echo-dpf/build-host" >&2
    exit 1
fi

if [[ ! -e "${JUCE_VST3}" ]]; then
    echo "ERROR: JUCE VST3 not found at ${JUCE_VST3}" >&2
    echo "       Build the JUCE original and set DUBWIZE_JUCE_VST3." >&2
    exit 1
fi

if ! command -v "${OFFLINE_HOST}" >/dev/null 2>&1; then
    echo "ERROR: offline host '${OFFLINE_HOST}' not found on PATH." >&2
    echo "       Install Carla (provides carla-single) or set OFFLINE_HOST to an" >&2
    echo "       alternative offline VST3 render host with the same interface:" >&2
    echo "         <host> vst3 <plugin.vst3> <in.wav> <out.wav> [param=value ...]" >&2
    exit 1
fi

# ── Generate fixture if missing ───────────────────────────────────────────────
if [[ ! -f "${FIXTURE}" ]]; then
    echo "--- Generating test fixture ---"
    python3 "${HERE}/fixtures/generate_input.py"
fi

# ── Preset definitions ────────────────────────────────────────────────────────
# Format: each preset is a bash associative-array declaration whose keys are
# parameter symbols (matching kParams[].symbol in ParameterMetadata.hpp) and
# whose values are the numeric values to apply.
#
# NOISE IS DISABLED IN ALL PRESETS (noiseEnabled=0, noiseLevel=-60) because
# the noise path is non-deterministic (OS PRNG seeding) and would make
# bit-exact comparison impossible.
#
# The carla-single parameter interface expects key=value pairs appended after
# the output WAV path.  See: carla-single --help

declare -A PRESET_DEFAULT=(
    [noiseEnabled]=0
    [noiseLevel]=-60
    # All other params at their documented defaults (plugin initialises to them)
    # Explicitly set a few to ensure both hosts agree:
    [mix]=50
    [time]=200
    [feedback]=65
    [toneType]=0
    [tapeDrive]=0
    [delayEnabled]=1
    [modEnabled]=1
    [filterEnabled]=1
    [decimEnabled]=1
    [bmEnabled]=0
)

declare -A PRESET_TAPE_FB=(
    [noiseEnabled]=0
    [noiseLevel]=-60
    # Tape feedback: TAPE tone type, high feedback, tape drive engaged
    [toneType]=1            # TAPE
    [feedback]=90
    [tapeDrive]=8
    [tapeComp]=60
    [tapeAge]=70
    [tapeBias]=0.2
    [mix]=70
    [time]=350
    [delayEnabled]=1
    [modEnabled]=0
    [filterEnabled]=1
    [decimEnabled]=0
    [bmEnabled]=0
)

declare -A PRESET_BITMOD_DECIM=(
    [noiseEnabled]=0
    [noiseLevel]=-60
    # BitMod XOR + decimation
    [bmEnabled]=1
    [bmOperation]=1         # XOR
    [bmOperands]=0          # POST FX + POST FX
    [bmLevel]=-12
    [bcDepth]=1.5
    [decimEnabled]=1
    [decimReduction]=0.3
    [decimStereoSpread]=0.2
    [toneType]=0            # DIGITAL
    [feedback]=50
    [mix]=60
    [filterEnabled]=0
    [modEnabled]=0
)

declare -A PRESET_FILTER_SWEEP=(
    [noiseEnabled]=0
    [noiseLevel]=-60
    # HPF and LPF moved to non-trivial positions
    [filterEnabled]=1
    [hpfCutoff]=200
    [hpfQ]=1.4
    [hpfPosition]=1         # POST BITMOD
    [lpfCutoff]=8000
    [lpfQ]=0.7
    [lpfPosition]=2         # FEEDBACK
    [filterGainComp]=1
    [filterSoftClip]=1
    [mix]=50
    [feedback]=55
    [toneType]=0
    [modEnabled]=1
    [modRate]=0.5
    [modDepth]=15
    [bmEnabled]=0
    [decimEnabled]=0
)

# Ordered list of preset names and per-preset threshold overrides.
# To relax a preset, add: PRESET_THRESHOLD[my_preset]=-60
# and document the numerical justification in a comment.
declare -a PRESET_NAMES=( default tape_fb bitmod_decim filter_sweep )
declare -A PRESET_THRESHOLD=(
    # [tape_fb]=-60  # example: tape saturation adds up to 0.5 LSB rounding
)

# ── Render function ───────────────────────────────────────────────────────────
# render_with_host <vst3_path> <in_wav> <out_wav> <assoc_array_name>
render_with_host() {
    local vst3="$1"
    local in_wav="$2"
    local out_wav="$3"
    local -n _params="$4"   # nameref to associative array

    # Build the param=value argument list
    local param_args=()
    for key in "${!_params[@]}"; do
        param_args+=( "${key}=${_params[$key]}" )
    done

    echo "    host: ${OFFLINE_HOST} vst3 ${vst3} → ${out_wav}"
    "${OFFLINE_HOST}" vst3 "${vst3}" "${in_wav}" "${out_wav}" "${param_args[@]}"
}

# ── Main loop ─────────────────────────────────────────────────────────────────
PASS=0
FAIL=0
FAIL_LIST=()

for preset_name in "${PRESET_NAMES[@]}"; do
    echo ""
    echo "=========================================="
    echo "Preset: ${preset_name}"
    echo "=========================================="

    # Resolve the params array by name (nameref approach via eval for portability)
    assoc_name="PRESET_$(echo "${preset_name}" | tr '[:lower:]' '[:upper:]')"

    out_juce="${OUTDIR}/out_juce_${preset_name}.wav"
    out_dpf="${OUTDIR}/out_dpf_${preset_name}.wav"

    echo "  Rendering JUCE original..."
    render_with_host "${JUCE_VST3}" "${FIXTURE}" "${out_juce}" "${assoc_name}"

    echo "  Rendering DPF port..."
    render_with_host "${DPF_VST3}" "${FIXTURE}" "${out_dpf}" "${assoc_name}"

    # Per-preset threshold (fall back to global)
    thresh="${THRESHOLD_DB}"
    if [[ -n "${PRESET_THRESHOLD[$preset_name]+_}" ]]; then
        thresh="${PRESET_THRESHOLD[$preset_name]}"
        echo "  NOTE: using relaxed threshold ${thresh} dB for preset '${preset_name}'"
    fi

    echo "  Comparing..."
    if python3 "${COMPARE_PY}" "${out_juce}" "${out_dpf}" --threshold-db "${thresh}"; then
        echo "  --> PASS"
        (( PASS++ )) || true
    else
        echo "  --> FAIL"
        (( FAIL++ )) || true
        FAIL_LIST+=( "${preset_name}" )
    fi
done

# ── Summary ───────────────────────────────────────────────────────────────────
echo ""
echo "=========================================="
echo "NULL-TEST SUMMARY"
echo "=========================================="
echo "  Passed: ${PASS} / $(( PASS + FAIL ))"
if [[ ${FAIL} -eq 0 ]]; then
    echo "  Result: PASS — all presets ≤ ${THRESHOLD_DB} dB peak residual"
    exit 0
else
    echo "  Result: FAIL — ${FAIL} preset(s) exceeded threshold:"
    for p in "${FAIL_LIST[@]}"; do
        echo "    • ${p}"
    done
    echo ""
    echo "  Next steps:"
    echo "    1. Re-run with THRESHOLD_DB=-60 to see how far off the preset is."
    echo "    2. If the residual has a numerical cause (rounding, denormal flushing,"
    echo "       plugin-init order), document it and add a per-preset threshold"
    echo "       override in PRESET_THRESHOLD[] with a comment."
    echo "    3. Otherwise investigate the DSP divergence between JUCE and DPF."
    exit 1
fi
